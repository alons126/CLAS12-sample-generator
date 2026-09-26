#!/usr/bin/env python3

#
# Created by Alon Sportes on 21/09/2026.
#

"""Preview or submit completed LUND samples on ifarm.

Purpose:
    Pass checked LUND and detector settings to the external GEMC/reconstruction worker through Slurm.

Workflow:
    Resolve inputs -> check and load GEMC -> check worker inputs -> prepare output only with --execute
    -> submit one array per sample -> record each accepted Slurm job ID in its submission log.
    Python changes a separate copy of the shell environment, so the user's interactive module setup
    stays unchanged. Detector commands remain in the external worker.

Inputs:
    Completed LUND files and manifests, optional config/CLI overrides, GCARD and YAML files,
    and the ifarm shell environment containing the module command, Slurm, reconstruction tools,
    and the shared ``*_COLOR`` settings. Standard GEMC selections also require the matching shared
    clas12Tags version directory; --clas12tags-dir supplies an explicit data override.

Outputs:
    Preview prints checks and commands without changing output. --execute replaces simulation output,
    calls Slurm, reports its accepted job ID, and writes the submission log while preserving lundfiles.

CLI options (parsed by resolve_inputs.py):
    --lund-dir DIRECTORY          Select completed RUN/lundfiles; repeat for multiple samples.
    --config FILE                 Read optional key = value submission settings.
    --execute                     Replace simulation outputs and submit; default: preview.
    --source uniform|physical     Set source when no manifest supplies it.
    --beam-energy GeV             Set truth beam energy when no manifest supplies it.
    --rgm-target ID               Set truth target identity when no manifest supplies it.
    --channel NAME                Set uniform 1e, eh, electron-tester, or a legacy label.
    --hadron NAME                 Set proton, neutron, pip, or pim for eh.
    --hadron-region FD|CD         Select the eh hadron detector region.
    --event-generator NAME        Set physical input adapter; default: genie-gst without a manifest.
    --tune NAME                   Set physical tune; default: unknown without a manifest.
    --q2-cut NAME                 Record physical input Q2 label; no cut is applied here.
    --prefix NAME                 Set LUND filename prefix; required without a manifest.
    --gemc-version VERSION        Select GEMC resources; fallback default: 5.14.
    --gemc-target-variation NAME  Select detector target variation.
    --gcard FILE / --yaml FILE    Override detector and reconstruction inputs.
    --torus SCALE                 Override the beam-dependent torus default.
    --num-jobs N                  Select first N LUND files; default: all completed files.
    --events-per-job N            Set event limit; required without a manifest.
    --job-name NAME               Override the metadata-derived Slurm job name.
    --clas12tags-dir DIRECTORY    Use a custom clas12Tags checkout as GEMC_DATA_DIR.
    --clear-farm-out true|false   Clear direct farm log files with --execute; default: false.
    --farm-out DIRECTORY          Set farm_out directory when clearing it.
    --fc-status 0|1               Set legacy physical filename/report label; default: 0.
    --help                        Print submission help before any server synchronization.

Failure:
    Missing module versions, inconsistent GEMC paths, missing inputs, unsafe outputs, and rejected
    submissions return nonzero and stop subsequent samples. Arrays already accepted by Slurm are
    never canceled. Invoke through run.csh so colors and the ifarm environment are available.
"""

from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from resolve_inputs import parser, path_value, resolve_samples

# Error presentation ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Error presentation
# run.csh loads the shared environment palette before starting submission.
ERROR_COLOR = os.environ.get('ERROR_COLOR', '').replace(r'\033', '\033')
RESET_COLOR = os.environ.get('RESET_COLOR', '').replace(r'\033', '\033')

def print_error(message):
    """Print one prefix-free message with the standard colored error label."""

    print(f'{ERROR_COLOR}Error: {RESET_COLOR}{message}', file=sys.stderr)
# endregion

# Submission record ----------------------------------------------------------

# region Submission record
def git_information(root):
    """Read Git details without changing the checkout.

    Purpose:
        Record the same main Git details as LUND creation.

    Args:
        root: Verified project checkout passed to Git as its working directory.

    Returns:
        A dictionary ready for JSON. Missing optional values are written as unavailable.
    """

    def read(*arguments, empty='Not available'):
        """Return one stripped Git result or the requested unavailable value."""

        result = subprocess.run(['git', *arguments], cwd=root, capture_output=True, text=True)

        if result.returncode:
            return 'Not available'

        return result.stdout.rstrip() or empty

    repository = read('remote', 'get-url', 'origin')
    branch = read('branch', '--show-current')
    commit_hash = read('rev-parse', 'HEAD')
    status = read('status', '--porcelain', empty='clean').replace('\n', ' | ')
    tracking = read('rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}')
    counts = read('rev-list', '--left-right', '--count', 'HEAD...@{upstream}')
    ahead = behind = 'Not available'

    if counts != 'Not available':
        fields = counts.split()

        if len(fields) == 2 and all(field.isdigit() for field in fields):
            ahead, behind = fields

    github_files_url = 'Not available'

    if repository.startswith('https://github.com/'):
        github_path = repository.removeprefix('https://github.com/').removesuffix('.git')
        github_files_url = f'https://github.com/{github_path}/tree/{commit_hash}'
    elif repository.startswith('git@github.com:'):
        github_path = repository.removeprefix('git@github.com:').removesuffix('.git')
        github_files_url = f'https://github.com/{github_path}/tree/{commit_hash}'

    return {
        'repository': repository,
        'branch': branch,
        'commit_message': read('log', '-1', '--format=%s'),
        'full_commit_hash': commit_hash,
        'commit_datetime': read('log', '-1', '--format=%cI'),
        'commit_author': read('log', '-1', '--format=%an'),
        'status_porcelain_summary': status,
        'nearest_tag': read('describe', '--tags', '--always', '--long'),
        'head_detached': branch == 'Not available',
        'tracking_branch': tracking,
        'tracking_ahead': ahead,
        'tracking_behind': behind,
        'github_files_url': github_files_url,
    }

def file_sha256(path):
    """Return the SHA-256 hash of one checked input file."""

    digest = hashlib.sha256()

    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)

    return digest.hexdigest()

def write_submission_log(values, environment, root, executables, command, job_id):
    """Write the exact submission settings for an accepted Slurm array.

    Args:
        values: Fully resolved sample and detector settings returned by the resolver.
        environment: Final environment dictionary that will be passed to ``sbatch``.
        root: Verified checkout containing the submitted worker script.
        executables: Verified absolute paths for GEMC, reconstruction, and sbatch.
        command: Exact argument list passed to ``subprocess.run``.
        job_id: Numeric identifier read from Slurm's successful submission response.

    Returns:
        Path to the completed JSON log in ``OUTPATH/reconhipo``.

    Failure:
        Hashing or final file replacement errors stop later samples. The accepted array remains
        submitted if writing the log fails after Slurm accepts the job.
    """

    parameters = dict(values)
    parameters.update({key: environment[key] for key in (
        'RUNNING_DIR', 'SUBMIT_SCRIPT_FILE', 'GEMC_DATA_DIR', 'ARRAY', 'SBATCH_EXPORT', 'SLURM_EXPORT_ENV')})
    parameters.update({name: str(path) for name, path in executables.items()})
    payload = Path(environment['SUBMIT_SCRIPT_FILE'])
    inputs = {
        'gcard': {'path': values['GCARD_FILE'], 'sha256': file_sha256(values['GCARD_FILE'])},
        'reconstruction_yaml': {'path': values['YAML_FILE'], 'sha256': file_sha256(values['YAML_FILE'])},
        'worker_payload': {'path': str(payload), 'sha256': file_sha256(payload)},
    }
    data = {
        'schema_version': 2,
        'workflow': 'slurm-submission',
        'created_datetime': datetime.now().astimezone().isoformat(timespec='seconds'),
        'slurm_job_id': job_id,
        'parameters': dict(sorted(parameters.items())),
        'command': command,
        'git': git_information(root),
        'inputs': inputs,
    }
    completed = Path(values['OUTPATH']) / 'reconhipo/slurm-submission-log.json'
    temporary = completed.with_suffix('.json.tmp')

    temporary.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')
    temporary.replace(completed)

    return completed
# endregion Submission record

# Status presentation --------------------------------------------------------

# region Status presentation
def status_banner(name, root):
    """Print one shared workflow status banner without changing the result.

    Args:
        name: Trusted printer suffix, either ``success`` or ``stop``.
        root: Verified checkout containing the shared presentation scripts.

    Outputs:
        Prints the shared artwork, or a plain-text fallback when tcsh cannot start.

    Failure:
        A printer error is ignored so presentation cannot replace the workflow status.
    """

    printer = root / 'src/launcher/presentation' / f'print_{name}.csh'

    # Keep artwork after all earlier buffered report text, including in captured logs.
    sys.stdout.flush()
    sys.stderr.flush()

    try:
        subprocess.run(['tcsh', '-f', str(printer)], cwd=root, check=False)
    except OSError:
        print(f'CLAS12 samples: {name}', flush=True)
# endregion Status presentation

# Reporting ------------------------------------------------------------------

# region Reporting
class Report:
    """Print the submission report with the inherited terminal colors.

    Purpose:
        Use the same messages in preview and execution without defining colors here.

    Use:
        One Report copies the six color values and prints text. It stores no workflow state.

    Output:
        Banners are 100 columns wide. Summary values align to the border; path checks stay compact.

    Failure:
        Construction rejects missing color settings. check() prints the missing path
        before raising so the caller can stop without a duplicate diagnostic.
    """

    # Use one width for borders, titles, and aligned summary values.
    BANNER_WIDTH = 100

    def __init__(self, environment):
        """Copy the colors supplied by the sourced launcher.

        Args:
            environment: Environment dictionary containing every required ``*_COLOR`` value.

        Failure:
            Missing colors raise ValueError before any report is printed.
        """

        # Decode the values from set_colors.csh once.
        names = ('ERROR', 'COMPLETION', 'SYSTEM', 'INFO', 'WARNING', 'RESET')

        if any(name + '_COLOR' not in environment for name in names):
            raise ValueError('printout colors are unavailable; source the submission workflow through run.csh.')

        self.colors = {name: environment[name + '_COLOR'].replace(r'\033', '\033') for name in names}

    def text(self, text=''):
        """Print one line after replacing known color markers.

        Args:
            text: Text containing zero or more semantic markers. Embedded newlines are retained;
                an omitted argument prints one blank line.

        Output:
            One print operation on standard output. Unknown braces remain ordinary text, which
            allows paths, commands, and external diagnostics to pass through unchanged.
        """

        # Replace only the six supported markers, then print one newline.
        for name, value in self.colors.items():
            text = text.replace('{' + name + '}', value)

        print(text)

    def error(self, message):
        """Print one prefix-free message with the standard colored error label."""

        self.text('{ERROR}Error: {RESET}' + str(message))

    def warning(self, message):
        """Print one prefix-free message with the standard colored warning label."""

        self.text('{WARNING}Warning: {RESET}' + str(message))

    def banner(self, title, main=False):
        """Print a main or subsection banner.

        Args:
            title: Visible heading placed between colored borders.
            main: Use slash borders when true, equals borders otherwise.

        Output:
            Three lines with 100-character top and bottom borders. Byte-length padding
            keeps the legacy heading alignment for the titles used by this workflow.
        """

        # Center the title and place an extra padding space on the right when needed.
        padding = self.BANNER_WIDTH - 4 - len(title.encode())
        left = max(0, padding // 2)
        right = max(0, padding - padding // 2)
        border, opening, closing = ('/', '//', '//') if main else ('=', '= ', ' =')

        self.text('{SYSTEM}' + border * self.BANNER_WIDTH + '{RESET}')
        self.text('{SYSTEM}' + opening + ' ' * left + '{RESET}' + title + '{SYSTEM}' + ' ' * right + closing + '{RESET}')
        self.text('{SYSTEM}' + border * self.BANNER_WIDTH + '{RESET}')

    def value(self, name, value, color='SYSTEM', value_color='RESET'):
        """Print a label and align its value with the banner edge.

        Args:
            name: Visible label printed before the colon.
            value: Value converted to text; an empty value receives no trailing padding.
            color: Name of the shared color used for the label.
            value_color: Name of the shared color used for the value.

        Output:
            A summary line whose nonempty value ends at BANNER_WIDTH - 1. Values longer than
            the available width keep one separating space and may extend past the banner.
            Color markers do not contribute to the visible width.
        """

        value = str(value)
        spaces = max(1, self.BANNER_WIDTH - 1 - len(name) - 1 - len(value)) if value else 0

        self.text('{' + color + '}' + name + ':{RESET}' + ' ' * spaces + '{' + value_color + '}' + value + '{RESET}')

    def check(self, name, path, directory=False):
        """Print and check one required file or directory.

        Purpose:
            Show the exact path before deciding whether it exists.

        Args:
            name: Label printed beside the path status.
            path: Required filesystem path.
            directory: Check for a directory when true, a file otherwise.

        Output:
            The compact ``NAME: path`` line, a description of the check, and a colored success or
            failure result. Successful directory checks add one blank separator line.

        Failure:
            Print the specific missing-path message, then raise RuntimeError so no dependent
            cleanup or Slurm submission can proceed.
        """

        kind = 'directory' if directory else 'file'

        # Print long paths from the left so they remain readable.
        self.text('{SYSTEM}' + name + ':{RESET} ' + str(path))
        self.text('{SYSTEM}--> Checking if {RESET}' + name + '{SYSTEM} is a ' + kind + '...{RESET}')

        if not (Path(path).is_dir() if directory else Path(path).is_file()):
            self.error('the following ' + kind + ' does not exist: ' + path)

            raise RuntimeError()  # The precise failure has already been printed.

        self.text('{SYSTEM}-->{RESET} {COMPLETION}' + name + ' exists.{RESET}')

        if directory:
            self.text()
# endregion Reporting

# Setup and submission -------------------------------------------------------

# region Submission
# Standard ifarm GEMC versions live below this shared clas12Tags directory. A loaded module may provide
# the same base path. Custom --clas12tags-dir values do not use this default.
IFARM_CLAS12TAGS_BASE = Path('/u/scigroup/cvmfs/geant4/almalinux9-gcc11/clas12Tags')

def check_gemc_version(version, environment, report):
    """Check that the requested ifarm clas12Tags version exists before changing modules.

    Purpose:
        Check that the requested GEMC data exists before changing the loaded GEMC module.

    Workflow:
        Find the shared clas12Tags directory from GEMC_DATA_DIR. If GEMC_DATA_DIR is missing, use the
        normal ifarm location. Check the shared directory and the requested version before loading
        the module, so a missing version does not disturb the current module setup.

    Args:
        version: Validated GEMC version requested for the sample.
        environment: Environment copied from the sourced ifarm launcher before loading GEMC.
        report: Helper that prints the path checks.

    Returns:
        The checked, version-specific directory expected to become GEMC_DATA_DIR after loading
        the module. verify_gemc() later compares the module result with this exact path.

    Failure:
        A missing base or version directory raises before the active GEMC module is changed.
    """

    # Remove the active version name to find the shared base, or use the site default.
    active_data = environment.get('GEMC_DATA_DIR')
    base = Path(active_data).parent if active_data else IFARM_CLAS12TAGS_BASE
    requested = base / version

    # Check the shared directory and requested version separately before unloading GEMC.
    report.check('CLAS12TAGS_BASE_DIR', str(base), directory=True)
    report.check('REQUESTED_GEMC_DATA_DIR', str(requested), directory=True)

    # load_gemc() changes the private environment; verify_gemc() checks the result against this path.
    return requested

def load_gemc(version, environment, report):
    """Load one GEMC module into the environment dictionary passed to Slurm.

    Purpose:
        Reproduce ``module unload gemc`` followed by ``module load gemc/VERSION`` without
        modifying the interactive shell that sourced run.csh.

    Workflow:
        Find modulecmd in PATH -> start a small Python process with the copied environment -> ask
        modulecmd to unload and load GEMC -> apply those changes in that process -> return its final
        environment as JSON -> check the JSON and replace the copied dictionary. Module messages stay
        connected to the terminal, so their original text and colors remain visible.

    Args:
        version: Validated GEMC module version selected for this sample.
        environment: Environment dictionary updated in place for this command.
        report: Helper that prints the version change.

    Failure:
        A missing module command, failed unload/load, or invalid JSON raises ValueError before output
        replacement or job submission. The dictionary changes only after the complete result passes
        validation, so a failed helper cannot leave a partly changed environment.
    """

    # Print the requested version before module commands can fail.
    report.text('{SYSTEM}Switching GEMC version to {RESET}{INFO}' + version + '{RESET}{SYSTEM}...{RESET}')

    # Find modulecmd because the interactive `module` name is a shell function.
    modulecmd = shutil.which('modulecmd', path=environment.get('PATH'))

    if modulecmd is None:
        raise ValueError('modulecmd is unavailable; the selected GEMC module cannot be loaded.')

    # A helper process runs unload and load, then prints its final environment as JSON. Module messages
    # remain visible on stderr.
    helper = ('import json, os, subprocess, sys\n'
              'for arguments in (("unload", "gemc"), ("load", "gemc/" + sys.argv[2])):\n'
              '    result = subprocess.run([sys.argv[1], "python", *arguments], stdout=subprocess.PIPE, text=True)\n'
              '    if result.returncode:\n'
              '        raise SystemExit(result.returncode)\n'
              '    exec(compile(result.stdout, sys.argv[1], "exec"), {"os": os})\n'
              'print(json.dumps(dict(os.environ)))\n')

    # Flush the report before module messages are printed.
    sys.stdout.flush()

    # Give the helper the copied environment and capture its final JSON output.
    result = subprocess.run([sys.executable, '-c', helper, modulecmd, version], env=environment,
                            stdout=subprocess.PIPE, stderr=sys.stdout, text=True)

    # On failure, keep the copied environment unchanged.
    if result.returncode:
        raise ValueError('failed to load GEMC module ' + version + '.')

    # Read the new environment only after both module commands succeed.
    try:
        loaded = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise ValueError('GEMC module loading returned an invalid environment.') from error

    # Require a dictionary of strings before replacing the working environment.
    if not isinstance(loaded, dict) or not all(isinstance(key, str) and isinstance(value, str)
                                               for key, value in loaded.items()):
        raise ValueError('GEMC module loading returned an invalid environment.')

    # Replace the dictionary in place so later checks and sbatch use the loaded module.
    environment.clear()
    environment.update(loaded)

    # Separate module messages from the next report section.
    report.text()

def verify_gemc(version, expected_data, environment, report):
    """Check the loaded GEMC data path and program before calling Slurm.

    Purpose:
        Check that loading ``gemc/VERSION`` selected the matching detector data and GEMC program.
        This catches an old PATH, a broken module file, or data left from another version.

    Workflow:
        Require GEMC_DATA_DIR -> make its path absolute and resolve links -> compare it with the path
        checked before loading -> require its last directory to equal VERSION -> find ``gemc`` in the
        loaded PATH -> require that program to be inside the same GEMC directory -> print its path.

    Args:
        version: Validated GEMC version requested for the sample.
        expected_data: Prechecked standard ifarm version directory, or None when a custom
            CLAS12TAGS_DIR will replace module data after executable validation.
        environment: Environment returned by the module command.
        report: Helper that prints the executable check.

    Returns:
        Checked absolute path to the GEMC program that Slurm will use.

    Failure:
        Missing or mismatched module data, or a GEMC program outside that data directory, raises before
        simulation outputs are replaced. A custom CLAS12TAGS_DIR is applied only after this check.
    """

    # A loaded GEMC module must provide GEMC_DATA_DIR.
    loaded_data_text = environment.get('GEMC_DATA_DIR')

    if not loaded_data_text:
        raise ValueError('loading GEMC ' + version + ' did not set GEMC_DATA_DIR.')

    # Resolve links and relative parts before comparing paths.
    loaded_data = Path(loaded_data_text).resolve()

    # A standard module must select the exact directory checked before loading.
    if expected_data is not None and loaded_data != expected_data.resolve():
        raise ValueError(f'loading GEMC {version} selected unexpected GEMC_DATA_DIR: {loaded_data}')

    # Also require the final directory name to match the requested version.
    if loaded_data.name != version:
        raise ValueError(f'loading GEMC {version} selected mismatched GEMC_DATA_DIR: {loaded_data}')

    # Find GEMC from the newly loaded PATH.
    executable_text = shutil.which('gemc', path=environment.get('PATH'))

    if executable_text is None:
        raise ValueError(f'gemc is unavailable after loading GEMC module {version}.')

    # Resolve links before checking that the program belongs to this installation.
    executable = Path(executable_text).resolve()

    # Require the resolved GEMC program to be inside GEMC_DATA_DIR.
    if loaded_data != executable and loaded_data not in executable.parents:
        raise ValueError(f'loading GEMC {version} selected an executable outside {loaded_data}: {executable}')

    # Show and check the exact GEMC program that Slurm will inherit.
    report.check('SLURM_GEMC_EXECUTABLE', str(executable))
    report.text()

    # Return the checked absolute program path.
    return executable

def clear_farm(values, root, execute, report, cleared):
    """Handle optional farm_out cleanup once for the full command.

    Purpose:
        Preserve the legacy ability to clear old ifarm log files without touching LUND or
        simulation output directories.

    Workflow:
        Report a disabled or previously handled request; otherwise validate the resolved
        farm_out directory. Preview describes the cleanup, while execution removes only
        direct regular files and leaves subdirectories and symbolic links in place.

    Args:
        values: Resolved settings containing CLEAR_FAR_OUT and farm_out.
        root: Checkout path used to reject unsafe cleanup destinations.
        execute: Whether this command may modify files.
        report: Helper that prints colored output.
        cleared: Whether an earlier sample already handled this request.

    Returns:
        Updated cleanup state for this command. Preview also marks the request handled so
        a multi-sample preview reports the proposed cleanup only once.

    Failure:
        A missing, linked, broad, or non-farm_out destination raises before any deletion.
    """

    # Do nothing when cleanup is off or was already handled for another sample.
    if values['CLEAR_FAR_OUT'] == 'false':
        return cleared

    if cleared:
        report.text('farm_out was already cleared for this command. Preserve newly created job logs.')
    else:
        farm = Path(values['farm_out'])

        # Before deletion, reject roots, checkout parents, links, and unrelated directories again.
        if (not farm.is_dir() or farm.is_symlink() or farm in (Path('/'), Path.home().resolve(), root)
                or farm in root.parents or 'farm_out' not in farm.parts):
            raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

        # Remove direct regular files only. Preview reports them without deleting.
        if execute:
            for entry in farm.iterdir():
                if not entry.is_symlink() and entry.is_file():
                    entry.unlink()
        else:
            report.text(f'PREVIEW: would clear files in {farm}')

        cleared = True

    report.text()

    return cleared

def submit_array(command, environment, root):
    """Submit one Slurm array and return its numeric job identifier.

    Workflow:
        Capture the scheduler response -> reproduce its standard output and error -> require a
        successful status -> extract the ID from ``Submitted batch job NUMBER``.

    Args:
        command: Exact checked sbatch argument list.
        environment: Final environment dictionary passed to the scheduler.
        root: Verified checkout used as the child working directory.

    Returns:
        The numeric Slurm job identifier as text, preserving its value exactly for reporting and
        the JSON submission log.

    Failure:
        A rejected command or successful response without the standard numeric notice raises and
        stops later samples. A scheduler response that cannot be parsed may still represent an
        accepted array, so the error states that operators must inspect Slurm before retrying.
    """

    result = subprocess.run(command, env=environment, cwd=root, capture_output=True, text=True)

    # Copy the scheduler's own notice and errors into the workflow log.
    if result.stdout:
        print(result.stdout, end='' if result.stdout.endswith('\n') else '\n')

    if result.stderr:
        print(result.stderr, end='' if result.stderr.endswith('\n') else '\n', file=sys.stderr)

    if result.returncode:
        raise ValueError('sbatch failed; no subsequent sample was submitted.')

    match = re.search(r'(?m)^Submitted batch job\s+([0-9]+)\s*$', result.stdout)

    if match is None:
        raise ValueError('sbatch returned success without a numeric job ID; inspect Slurm before retrying.')

    return match.group(1)

def submit_sample(values, environment, root, execute, report, farm_cleared):
    """Check, report, and optionally submit one completed LUND sample.

    Purpose:
        Pass one checked sample to the external GEMC/reconstruction Slurm worker.

    Workflow:
        1. Add the checked sample settings and fixed checkout paths to the environment dictionary.
        2. Print the sample details and handle the optional one-time farm-log cleanup.
        3. Check standard GEMC data, load the selected module, and verify its data and program.
        4. Apply an optional custom clas12Tags data override and validate detector inputs.
        5. Recheck every selected LUND input and required executable before output replacement.
        6. Preserve outputs in preview or recreate only mchipo/reconhipo during execution.
        7. Print the array settings and call the external worker through sbatch only with --execute.
        8. Read and print the accepted job ID, then save it in the submission log.

    Args:
        values: One validated sample dictionary returned by resolve_samples().
        environment: Copy of os.environ used only by this command. Sample settings and the selected
            GEMC module replace matching values. The same copy is reused for later samples.
        root: Checkout directory containing the external worker script.
        execute: False for read-only preview; true for cleanup and Slurm submission.
        report: Helper that prints the report.
        farm_cleared: Whether this command already handled farm_out cleanup.

    Returns:
        Updated farm_out state for the next selected sample. This prevents repeated cleanup from
        deleting log files created by an earlier array in the same command.

    Failure:
        Missing inputs, unsafe output children, absent commands, or sbatch failure raise and
        stop later samples. All read-only checks occur before mchipo/reconhipo replacement.
        The submission log is written after Slurm accepts the array. Already accepted Slurm arrays
        are not canceled when log publication or a later sample fails.
    """

    # Copy worker values into the environment dictionary and add fixed checkout paths.
    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/workflows/slurm-submission/external/submit_GEMC_sample.sh'))

    uniform = values['source'] == 'uniform'

    # Preview and execution use the same checks and report.
    if not execute:
        report.text('{INFO}PREVIEW:{RESET}\nNo sbatch, output replacement or farm_out cleanup; add --execute to submit.')
        report.text()

    report.banner('Slurm submission workflow parameters')

    # Show the checkout, cleanup choice, and GEMC version before output can change.
    for key in ('RUNNING_DIR', 'CLEAR_FAR_OUT', 'GEMC_VERSION'):
        report.value(key, environment[key])

    identity = 'uniform' if uniform else 'physical (' + values['SAMPLE_GENERATOR'] + ')'

    report.value('Sample type', identity, value_color='INFO')

    farm_cleared = clear_farm(values, root, execute, report, farm_cleared)

    report.text()

    # Check standard shared clas12Tags data before changing modules. Custom data is checked later.
    expected_gemc_data = None

    # Custom clas12Tags skips only the shared-directory lookup; GEMC is still checked.
    if not values['CLAS12TAGS_DIR']:
        expected_gemc_data = check_gemc_version(values['GEMC_VERSION'], environment, report)

    # Load the module only after its files are found, then check its data path and program.
    load_gemc(values['GEMC_VERSION'], environment, report)
    gemc_executable = verify_gemc(values['GEMC_VERSION'], expected_gemc_data, environment, report)

    # Reapply checked sample values after the module changes the environment.
    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/workflows/slurm-submission/external/submit_GEMC_sample.sh'))

    # A checked custom clas12Tags directory replaces GEMC_DATA_DIR.
    if values['CLAS12TAGS_DIR']:
        report.check('CLAS12TAGS_DIR', values['CLAS12TAGS_DIR'], directory=True)

    # Whether loaded or overridden, GEMC_DATA_DIR must exist.
    if values['CLAS12TAGS_DIR']:
        environment['GEMC_DATA_DIR'] = values['CLAS12TAGS_DIR']

    if 'GEMC_DATA_DIR' not in environment:
        raise ValueError('GEMC_DATA_DIR is missing from the preloaded GEMC environment; select --clas12tags-dir for a custom checkout.')

    report.check('GEMC_DATA_DIR', environment['GEMC_DATA_DIR'], directory=True)

    # Show the resolved target, beam, and torus settings.
    report.banner('Sample parameters')

    for key in ('SAMPLE_TARGET_NUCLEUS', 'TARGET_VARIATION', 'BEAM_ENERGY_LABEL',
                'DETECTOR_ENERGY_GROUP', 'TORUS_FIELD'):
        report.value(key, values[key])

    report.text()

    # Keep sample identity together. The field-cage flag changes names, not event selection.
    if uniform:
        report.value('UNIFORM_SAMPLE_CHANNEL', values['UNIFORM_SAMPLE_CHANNEL'], color='INFO')
    else:
        for key in ('GENERATOR_TUNE', 'Q2_CUT', 'FC_STATUS', 'FC_STATUS_ENABLED'):
            report.value(key, values[key], color='INFO')

    report.text()

    # Check detector inputs before replacing simulation output.
    required_paths = [('OUTPATH', values['OUTPATH'], True),
                      ('REQUIREMENTS_DIR', values['REQUIREMENTS_DIR'], True),
                      ('GCARD_FILE', values['GCARD_FILE'], False),
                      ('YAML_FILE', values['YAML_FILE'], False),
                      ('SUBMIT_SCRIPT_FILE', environment['SUBMIT_SCRIPT_FILE'], False)]

    if not uniform:
        required_paths.insert(1, ('RUNNING_DIR', str(root), True))

    for name, path, directory in required_paths:
        if not (Path(path).is_dir() if directory else Path(path).is_file()):
            raise ValueError(f'missing required {"directory" if directory else "file"} {name}: {path}')

    # Recheck selected LUND files immediately before output replacement.
    run = Path(values['OUTPATH'])

    for index in range(1, int(values['NUM_OF_JOBS']) + 1):
        path = run / 'lundfiles' / f"{values['SAMPLE_FILE_PREFIX']}_{index}.txt"

        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f'missing or empty LUND input: {path}')

    # Preview checks detector programs but does not require sbatch.
    executables = {}

    for executable in (('sbatch', 'gemc', 'recon-util') if execute else ('gemc', 'recon-util')):
        resolved_executable = shutil.which(executable, path=environment.get('PATH'))

        if resolved_executable is None:
            raise ValueError(f'{executable} is unavailable in the loaded environment.')

        executables[{'gemc': 'SLURM_GEMC_EXECUTABLE',
                     'recon-util': 'SLURM_RECON_EXECUTABLE',
                     'sbatch': 'SBATCH_EXECUTABLE'}[executable]] = resolved_executable

    # verify_gemc() already proved that GEMC belongs to the requested installation.
    executables['SLURM_GEMC_EXECUTABLE'] = gemc_executable

    report.banner('Setting output directories' + (' for ' + values['UNIFORM_SAMPLE_CHANNEL'] if uniform else ''))

    # Only these simulation directories may be replaced. Reject links before deletion.
    output_dirs = [run / name for name in ('mchipo', 'reconhipo')]

    if any(path.is_symlink() for path in output_dirs):
        raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

    # Execution recreates simulation output directories. Preview only reports this action. Both keep
    # lundfiles unchanged.
    if execute:
        report.text('{INFO}Removing old directory structure for MC simulation here...{RESET}')

        for path in output_dirs:
            if path.is_dir():
                shutil.rmtree(path)
            elif path.exists():
                path.unlink()

        report.text('{INFO}Setting up directory structure for MC simulation here...{RESET}')

        for path in output_dirs:
            path.mkdir()

        report.text()
    else:
        report.text('{INFO}PREVIEW:{RESET} would replace mchipo reconhipo under ' + str(run) + '; existing outputs are preserved.')

    report.value('OUTPATH', str(run))
    report.text('{SYSTEM}Number of files in target directory (OUTPATH):{RESET}')

    # Report visible lundfiles entries without counting the monitoring directory.
    lund_count = sum(not path.name.startswith('.') for path in (run / 'lundfiles').iterdir()) - 1

    report.value('Number of lund files', lund_count)

    for name, label in (('mchipo', 'Number of mchipo files'), ('reconhipo', 'Number of reconhipo files')):
        directory = run / name
        count = sum(not path.name.startswith('.') for path in directory.iterdir()) if directory.is_dir() else 0

        report.value(label, count)

    report.text()

    # Each selected PREFIX_INDEX.txt file becomes one array task.
    report.banner('Slurm jobs parameters')

    report.value('NUM_OF_JOBS', values['NUM_OF_JOBS'])
    environment['ARRAY'] = '1-' + values['NUM_OF_JOBS']
    report.value('SLURM_JOB_NAME', values['SLURM_JOB_NAME'])
    report.value('ARRAY', environment['ARRAY'])
    report.text()

    report.check('OUTPATH', values['OUTPATH'], directory=True)

    for name in ('mchipo', 'reconhipo'):
        path = str(run / name)

        report.check(name, path, directory=True)

    if not uniform:
        report.check('RUNNING_DIR', str(root), directory=True)

    report.check('REQUIREMENTS_DIR', values['REQUIREMENTS_DIR'], directory=True)

    for key in ('GCARD_FILE', 'YAML_FILE'):
        report.check(key, values[key])
        report.text()

    payload = environment['SUBMIT_SCRIPT_FILE']
    report.check('SUBMIT_SCRIPT_FILE', payload)
    report.text()

    report.banner('Submitting sbatch job for ' + ('uniform' if uniform else values['SAMPLE_GENERATOR']) + ' sample')

    # Show the exact command in both modes and run it only with --execute.
    report.text('{SYSTEM}Submitted job with command:{RESET}' if execute else '{INFO}Preview command (not submitted):{RESET}')
    report.text('{SYSTEM}sbatch --job-name={RESET}' + values['SLURM_JOB_NAME'] + '{SYSTEM} --array={RESET}' + environment['ARRAY'] + ' ' + payload)

    if execute:
        # Flush report text before the captured scheduler response is reproduced.
        sys.stdout.flush()

        # Pass arguments directly, report the accepted ID, and save it in the submission log.
        command = ['sbatch', '--job-name=' + values['SLURM_JOB_NAME'], '--array=' + environment['ARRAY'], payload]
        job_id = submit_array(command, environment, root)
        report.value('SLURM_JOB_ID', job_id, value_color='INFO')
        write_submission_log(values, environment, root, executables, command, job_id)

    report.text()

    return farm_cleared
# endregion Submission

# Entry point ----------------------------------------------------------------

# region Entry point
def main():
    """Preview or submit every selected completed LUND sample.

    Purpose:
        Run the Python part called by run.csh after the shell environment is ready. Return one integer
        status without closing the interactive shell that sourced run.csh.

    Workflow:
        Copy the shell environment -> prepare the report -> read the command line -> check every sample
        before processing the first -> check the checkout and worker script -> process samples in order
        while reusing the copied module environment and cleanup state -> print the shared success or
        stop banner.

    Inputs:
        Process arguments and the ifarm environment, including the shared colors and
        module/reconstruction paths. The current working directory must be the verified checkout
        root because project files and the external worker path are relative to the checkout.

    Outputs:
        A complete preview or execution report on standard output. Execution creates fresh mchipo and
        reconhipo directories, submits arrays, and saves each accepted job ID in its submission log.
        Preview runs the same checks without changing files or submitting. Neither mode changes the
        environment in the caller's shell.

    Returns:
        Zero when every selected sample is previewed or submitted successfully; one for a
        handled setup, validation, module, filesystem, or scheduler failure.

    Failure:
        A failed sample stops later samples. Slurm arrays accepted earlier in the same
        command remain submitted. The shared stop banner appears before the error. Errors are
        printed through Report when available; failures before colors are ready use a plain
        standard-error fallback.
    """

    # The source location is stable even when configuration fails before checkout validation.
    root = Path(__file__).resolve().parents[3]

    # Use plain text if failure happens before terminal colors are available.
    report = None

    try:
        # Change a copied environment so the interactive shell stays unchanged.
        environment = dict(os.environ)
        report = Report(environment)

        report.banner('Running Slurm submission script', main=True)
        report.text()

        # Check every sample before the first one can change output.
        args = parser().parse_args()
        samples = resolve_samples(args, root)

        # Require the external worker at its expected safe checkout path.
        path_value(root / 'src/workflows/slurm-submission/external/submit_GEMC_sample.sh', 'SUBMIT_SCRIPT_FILE')

        if Path.cwd().resolve() != root or not (root / 'src/workflows/slurm-submission/external/submit_GEMC_sample.sh').is_file():
            raise ValueError('source the submission script from the CLAS12-sample-generator checkout.')

        # Reuse the copied environment and one-time farm cleanup across samples.
        farm_cleared = False

        for values in samples:
            farm_cleared = submit_sample(values, environment, root, args.execute, report, farm_cleared)

        status_banner('success', root)

        return 0

    except KeyboardInterrupt:
        status_banner('stop', root)

        if report:
            report.error('Interrupted.')
        else:
            print_error('Interrupted.')

        return 130
    except Exception as error:
        # Convert every remaining submission failure to one shell-visible diagnostic and status.
        status_banner('stop', root)

        if str(error):
            if report:
                report.error(error)
            else:
                print_error(error)

        return 1

# Return main's status to run.csh.
if __name__ == '__main__':
    sys.exit(main())
# endregion Entry point
