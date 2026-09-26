#!/usr/bin/env python3

#
# Created by Alon Sportes on 21/09/2026.
#

"""Preview or submit completed LUND samples on ifarm.

Purpose:
    Pass checked LUND and detector settings to the external GEMC/reconstruction worker through Slurm.

Workflow:
    Resolve inputs -> check and load GEMC -> check worker inputs -> prepare output only with --execute
    -> write the submission log -> submit one array per sample.
    Python changes a private copy of the shell environment, so the user's interactive module setup
    stays unchanged. Detector commands remain in the external worker.

Inputs:
    Completed LUND files and manifests, optional config/CLI overrides, GCARD and YAML files,
    and the ifarm shell environment containing the module command, Slurm, reconstruction tools,
    and the shared ``*_COLOR`` palette. Standard GEMC selections also require the matching shared
    clas12Tags version directory; --clas12tags-dir supplies an explicit data override.

Outputs:
    Preview prints checks and commands without changing output. --execute replaces simulation output,
    writes the submission log, and calls Slurm while preserving lundfiles.

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
import shutil
import subprocess
import sys

from resolve_inputs import parser, path_value, resolve_samples

# Provenance -----------------------------------------------------------------

# region Provenance
def git_information(root):
    """Read Git details without changing the checkout.

    Purpose:
        Record the same main Git details as LUND generation.

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

def write_submission_log(values, environment, root, executables, command):
    """Write the exact submission settings before calling Slurm.

    Args:
        values: Fully resolved sample and detector settings returned by the resolver.
        environment: Final private environment that will be passed to ``sbatch``.
        root: Verified checkout containing the submitted payload.
        executables: Verified absolute paths for GEMC, reconstruction, and sbatch.
        command: Exact argument list passed to ``subprocess.run``.

    Returns:
        Path to the completed JSON log in ``OUTPATH/reconhipo``.

    Failure:
        Hashing or final file replacement errors stop submission before Slurm is called.
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
        'schema_version': 1,
        'workflow': 'slurm-submission',
        'created_datetime': datetime.now().astimezone().isoformat(timespec='seconds'),
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
# endregion Provenance

# Reporting ------------------------------------------------------------------

# region Reporting
class Report:
    """Print the submission report with the inherited terminal colors.

    Purpose:
        Use the same messages in preview and execution without defining colors here.

    Lifecycle:
        One Report copies the six color values and prints text. It stores no workflow state.

    Output:
        Banners are 100 columns wide. Summary values align to the border; path checks stay compact.

    Failure:
        Construction rejects an incomplete color palette. check() prints the missing path
        before raising so the caller can stop without a duplicate diagnostic.
    """

    # Use one width for borders, titles, and aligned summary values.
    BANNER_WIDTH = 100

    def __init__(self, environment):
        """Copy the colors supplied by the sourced launcher.

        Args:
            environment: Invocation environment containing every required ``*_COLOR`` value.

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
            color: Semantic palette name used for the label.
            value_color: Semantic palette name used for the value.

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
            The compact ``NAME: path`` line, a description of the test, and a colored success or
            failure result. Successful directory checks add one blank separator line.

        Failure:
            Print the specific missing-path message, then raise RuntimeError so no dependent
            cleanup or Slurm handoff can proceed.
        """

        kind = 'directory' if directory else 'file'

        # Print long paths from the left so they remain readable.
        self.text('{SYSTEM}' + name + ':{RESET} ' + str(path))
        self.text('{SYSTEM}--> Checking if {RESET}' + name + '{SYSTEM} is a ' + kind + '...{RESET}')

        if not (Path(path).is_dir() if directory else Path(path).is_file()):
            self.text('{SYSTEM}-->{RESET} {ERROR}Error:{RESET} the following ' + kind + ' does not exist: ' + path)

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
        Prevent a requested standard GEMC version from replacing the working module environment
        when its matching clas12Tags data is unavailable.

    Workflow:
        Derive the shared clas12Tags base by removing the active version component from the
        inherited GEMC_DATA_DIR. When no active path is available, use the documented ifarm base.
        Check both that base and its requested-version child before module loading begins. This
        ordering preserves the caller's working module environment when the requested installation
        is absent instead of unloading GEMC and discovering the problem afterward.

    Args:
        version: Validated GEMC version requested for the sample.
        environment: Pre-load environment inherited from the sourced ifarm launcher.
        report: Shared renderer used for visible safety checks.

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
    """Load one GEMC module into the private environment passed to Slurm.

    Purpose:
        Reproduce ``module unload gemc`` followed by ``module load gemc/VERSION`` without
        modifying the interactive shell that sourced run.csh.

    Workflow:
        Locate modulecmd in the inherited PATH; start a short Python helper with the invocation's
        environment; ask modulecmd for Python environment mutations for unload and load; execute
        those mutations inside the helper; serialize the resulting environment as JSON; validate
        it and replace the invocation-owned environment dictionary. modulecmd diagnostics remain
        connected to the terminal so their original text and ANSI colors are preserved.

    Args:
        version: Validated GEMC module version selected for this sample.
        environment: Invocation-owned environment updated in place.
        report: Shared renderer used for the version-switch status message.

    Failure:
        A missing module command, rejected unload/load, or malformed environment result raises
        ValueError before output replacement or job submission. A failed helper never partially
        updates the coordinator's environment because replacement occurs only after JSON validation.
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

    # Give the helper the private environment and capture its final JSON output.
    result = subprocess.run([sys.executable, '-c', helper, modulecmd, version], env=environment,
                            stdout=subprocess.PIPE, stderr=sys.stdout, text=True)

    # On failure, keep the original private environment unchanged.
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
        Prove that loading ``gemc/VERSION`` changed both the detector-data directory and the
        executable search path, rather than trusting the requested module name alone. This guards
        against a stale PATH, a misconfigured modulefile, or a module that reports success while
        retaining resources from another version.

    Workflow:
        Require GEMC_DATA_DIR; resolve it canonically; compare it with the prechecked standard path
        when applicable; require its final component to equal VERSION; resolve ``gemc`` through the
        loaded PATH; require the executable to live inside that data tree; print the checked path.

    Args:
        version: Validated GEMC version requested for the sample.
        expected_data: Prechecked standard ifarm version directory, or None when a custom
            CLAS12TAGS_DIR will replace module data after executable validation.
        environment: Environment returned by the module command.
        report: Shared renderer used for the executable safety check.

    Returns:
        Canonical absolute GEMC executable path inherited by Slurm. The caller currently uses the
        return value as a verified contract result rather than a separate configuration input.

    Failure:
        Missing/mismatched module data or an executable outside that data tree raises before
        simulation outputs are replaced. An explicit custom CLAS12TAGS_DIR changes detector data
        only after this binary/module consistency check succeeds.
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
        execute: Whether this invocation may modify files.
        report: Shared colored output renderer.
        cleared: Whether an earlier sample already handled this request.

    Returns:
        Updated invocation-wide cleanup state. Preview also marks the request handled so
        a multi-sample preview reports the proposed cleanup only once.

    Failure:
        A missing, linked, broad, or non-farm_out destination raises before any deletion.
    """

    # Do nothing when cleanup is off or was already handled for another sample.
    if values['CLEAR_FAR_OUT'] == 'false':
        return cleared

    if cleared:
        report.text('farm_out was already cleared for this submission invocation. Preserve newly created job logs.')
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

def submit_sample(values, environment, root, execute, report, farm_cleared):
    """Check, report, and optionally submit one completed LUND sample.

    Purpose:
        Bridge a resolver-approved sample to the external GEMC/reconstruction Slurm payload.

    Workflow:
        1. Merge resolver-approved worker values and fixed checkout-owned exports.
        2. Report invocation identity and handle optional invocation-wide farm-log cleanup.
        3. Precheck standard GEMC data, load the selected module, and verify its data/executable.
        4. Apply an optional custom clas12Tags data override and validate detector inputs.
        5. Recheck every selected LUND input and required executable before output replacement.
        6. Preserve outputs in preview or recreate only mchipo/reconhipo during execution.
        7. Report the array contract, publish its provenance log, and call the external payload
            through sbatch only in execute.

    Args:
        values: One validated sample dictionary returned by resolve_samples().
        environment: Invocation-owned copy of os.environ. Sample exports and the selected GEMC
            module override inherited values. It is intentionally carried between samples so each
            subsequent module transition starts from the preceding private environment.
        root: Checkout directory containing the external worker payload.
        execute: False for read-only preview; true for cleanup and Slurm submission.
        report: Shared renderer for the legacy-style transcript.
        farm_cleared: Invocation-wide farm_out cleanup state.

    Returns:
        Updated farm_out state for the next selected sample. This prevents repeated cleanup from
        deleting log files created by an earlier array in the same invocation.

    Failure:
        Missing inputs, unsafe output children, absent commands, or sbatch failure raise and
        stop later samples. All read-only preflight checks occur before mchipo/reconhipo replacement.
        Provenance publication occurs after replacement and before sbatch. Already accepted Slurm
        arrays are not canceled when a later sample fails.
    """

    # Copy worker values into the private environment and add fixed checkout paths.
    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/slurm-submission/external/submit_GEMC_sample.sh'))

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
                       SUBMIT_SCRIPT_FILE=str(root / 'src/slurm-submission/external/submit_GEMC_sample.sh'))

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
        # Flush report text before sbatch writes output.
        sys.stdout.flush()

        # Pass arguments directly. A rejected submission stops later samples.
        command = ['sbatch', '--job-name=' + values['SLURM_JOB_NAME'], '--array=' + environment['ARRAY'], payload]
        write_submission_log(values, environment, root, executables, command)
        sys.stdout.flush()

        if subprocess.run(command, env=environment, cwd=root).returncode:
            raise ValueError('sbatch failed; no subsequent sample was submitted.')

    report.text()

    return farm_cleared
# endregion Submission

# Entry point ----------------------------------------------------------------

# region Entry point
def main():
    """Preview or submit every selected completed LUND sample.

    Purpose:
        Provide the process boundary called by run.csh after its shell environment is ready.
        Keep Python exceptions, per-sample state, and child environments behind one shell-visible
        integer status without terminating the interactive shell that sourced the launcher.

    Workflow:
        Copy the inherited environment and initialize reporting; parse the CLI; resolve and
        validate every requested sample before processing the first; verify the checkout and
        external payload boundary; process distinct samples in caller order while carrying the
        private module environment and farm-cleanup state; convert known operational failures into
        one nonzero status.

    Inputs:
        Process arguments and the ifarm environment, including the shared color palette and
        module/reconstruction paths. The current working directory must be the verified checkout
        root because maintained resource paths and the external payload are checkout-relative.

    Outputs:
        A complete preview or execution transcript on standard output. Execution creates fresh
        mchipo/reconhipo directories, publishes a submission-provenance log, and submits arrays;
        preview performs the same validation without those mutations. Neither mode exports its
        private environment back to the caller's shell.

    Returns:
        Zero when every selected sample is previewed or submitted successfully; one for a
        handled setup, validation, module, filesystem, or scheduler failure.

    Failure:
        A failed sample stops later samples. Slurm arrays accepted earlier in the same
        invocation remain submitted. Errors are printed through Report when available; failures
        before palette construction use a plain standard-error fallback.
    """

    # Use plain text if failure happens before terminal colors are available.
    report = None

    try:
        # Change a private environment copy so the interactive shell stays unchanged.
        environment = dict(os.environ)
        report = Report(environment)

        report.banner('Running Slurm submission script', main=True)
        report.text()

        # Check every sample before the first one can change output.
        args = parser().parse_args()
        root = Path(__file__).resolve().parents[2]
        samples = resolve_samples(args, root)

        # Require the external worker at its expected safe checkout path.
        path_value(root / 'src/slurm-submission/external/submit_GEMC_sample.sh', 'SUBMIT_SCRIPT_FILE')

        if Path.cwd().resolve() != root or not (root / 'src/slurm-submission/external/submit_GEMC_sample.sh').is_file():
            raise ValueError('source the submission script from the CLAS12-sample-generator checkout.')

        # Reuse the private environment and one-time farm cleanup across samples.
        farm_cleared = False

        for values in samples:
            farm_cleared = submit_sample(values, environment, root, args.execute, report, farm_cleared)

        return 0

    except (OSError, ValueError, TypeError, KeyError, RuntimeError) as error:
        # Convert expected errors to one shell-visible failure status.
        if str(error):
            if report:
                report.text('{ERROR}Error:{RESET} ' + str(error))
            else:
                print(f'Error: {error}', file=sys.stderr)

        return 1

# Return main's status to run.csh.
if __name__ == '__main__':
    sys.exit(main())
# endregion Entry point
