#!/usr/bin/env python3

#
# Created by Alon Sportes on 21/09/2026.
#

"""Preview or submit existing LUND samples through the protected ifarm payload.

Purpose:
    Coordinate detector-job submission for already completed uniform or physical LUND runs.
    resolve_inputs.py owns setting validation; the protected shell payload owns GEMC and
    reconstruction commands. This coordinator owns the boundary between those layers: it turns
    validated settings into one checked process environment and passes that environment to Slurm.

Workflow:
    resolve every input -> check/load/verify selected GEMC -> validate worker inputs ->
    prepare mchipo/reconhipo only with --execute -> submit one Slurm array per sample.
    The sourced shell supplies the shared palette and module command. Python copies that process
    environment, applies the selected GEMC module to the copy, verifies GEMC_DATA_DIR and the
    executable, then gives the result to sbatch. A child process cannot rewrite its parent shell,
    so the interactive shell deliberately retains the module selection it had before invocation.
    No detector commands are implemented here.

Inputs:
    Completed LUND files and manifests, optional config/CLI overrides, GCARD and YAML files,
    and the ifarm shell environment containing the module command, Slurm, reconstruction tools,
    and the shared COLOR_* palette. Standard GEMC selections also require the matching shared
    clas12Tags version directory; --clas12tags-dir supplies an explicit data override.

Outputs:
    Preview prints the intended checks and commands without replacing outputs or submitting.
    --execute replaces the selected simulation directories and submits Slurm arrays; it
    preserves lundfiles. Both modes print the module transition and the exact GEMC executable
    that their Slurm environment would inherit.

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
    --event-generator NAME        Set physical generator; default: genie without a manifest.
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

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from resolve_inputs import parser, path_value, resolve_samples

# Reporting ------------------------------------------------------------------

# region Reporting
class Report:
    """Render the existing shell transcript using the inherited COLOR_* palette.

    Purpose:
        Give preview and execution the same status, path-check, and failure messages without
        defining ANSI escapes or shell aliases in this module. Keep summary values visually
        aligned while leaving safety-check paths compact and readable.

    Lifecycle:
        One Report is created from the caller's environment. It owns a decoded copy of the
        six semantic color values; later methods write directly to standard output. The object
        owns presentation only and does not retain workflow state or filesystem results.

    Output:
        Banners retain the shared shell renderer's 100-column borders and asymmetric title
        padding. Summary values end one column before that border. Safety checks instead print
        ``NAME: path`` without banner alignment, followed by their check and result lines.
        Markers such as {START} are expanded only while printing, so escape sequences never
        affect width calculations.

    Failure:
        Construction rejects an incomplete color palette. check() prints the missing path
        before raising so the caller can stop without a duplicate diagnostic.
    """

    # One shared width drives banner borders, title centering, and summary-value alignment.
    BANNER_WIDTH = 100

    def __init__(self, environment):
        """Copy and decode the palette supplied by the sourced launcher.

        Args:
            environment: Invocation environment containing every required COLOR_* value.

        Failure:
            Missing colors raise ValueError before any report is printed.
        """

        # These semantic names match set_colors.csh. Python decodes the exported literal
        # backslash-033 prefix once, then every printing method reuses the same values.
        names = ('START', 'ERR', 'COMPLETION', 'INFO', 'WARNING', 'END')

        if any('COLOR_' + name not in environment for name in names):
            raise ValueError('printout colors are unavailable; source the submission workflow through run.csh.')

        self.colors = {name: environment['COLOR_' + name].replace(r'\033', '\033') for name in names}

    def text(self, text=''):
        """Print one line after substituting known semantic color markers.

        Args:
            text: Text containing zero or more semantic markers. Embedded newlines are retained;
                an omitted argument prints one blank line.

        Output:
            One print operation on standard output. Unknown braces remain ordinary text, which
            allows paths, commands, and external diagnostics to pass through unchanged.
        """

        # Replace only the six markers owned by this Report, then emit exactly one newline.
        for name, value in self.colors.items():
            text = text.replace('{' + name + '}', value)

        print(text)

    def banner(self, title, main=False):
        """Print a main or subsection banner using the shared shell layout.

        Args:
            title: Visible heading placed between colored borders.
            main: Use slash borders when true, equals borders otherwise.

        Output:
            Three lines with 100-character top and bottom borders. Byte-length padding
            keeps the legacy heading alignment for the titles used by this workflow.
        """

        # Split remaining title width across both sides; the asymmetric remainder
        # matches the existing shell printout rather than changing its visual contract.
        padding = self.BANNER_WIDTH - 4 - len(title.encode())
        left = max(0, padding // 2)
        right = max(0, padding - padding // 2)
        border, opening, closing = ('/', '//', '//') if main else ('=', '= ', ' =')

        self.text('{START}' + border * self.BANNER_WIDTH + '{END}')
        self.text('{START}' + opening + ' ' * left + '{END}' + title + '{START}' + ' ' * right + closing + '{END}')
        self.text('{START}' + border * self.BANNER_WIDTH + '{END}')

    def value(self, name, value, color='START', value_color='END'):
        """Print a label and value, aligning the value's end to banner column 99.

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

        self.text('{' + color + '}' + name + ':{END}' + ' ' * spaces + '{' + value_color + '}' + value + '{END}')

    def check(self, name, path, directory=False):
        """Check a required file or directory while preserving the shell transcript.

        Purpose:
            Couple every filesystem decision to the exact path visible to the operator. Unlike
            value(), this safety output is intentionally not right-aligned to the banner.

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

        # Safety-check paths use compact left-aligned output so they remain easy to read even
        # when the path is long. The message precedes the check so failures retain context.
        self.text('{START}' + name + ':{END} ' + str(path))
        self.text('{START}--> Checking if {END}' + name + '{START} is a ' + kind + '...{END}')

        if not (Path(path).is_dir() if directory else Path(path).is_file()):
            self.text('{START}-->{END} {ERR}Error:{END} the following ' + kind + ' does not exist: ' + path)

            raise RuntimeError()  # The precise failure has already been printed.

        self.text('{START}-->{END} {COMPLETION}' + name + ' exists.{END}')

        if directory:
            self.text()
# endregion Reporting

# Setup and submission -------------------------------------------------------

# region Submission
# Standard ifarm GEMC modules publish one version directory beneath this shared clas12Tags root.
# A currently loaded GEMC_DATA_DIR may supply the same root dynamically, but this constant provides
# the site fallback when no GEMC module is active yet. Custom --clas12tags-dir inputs do not use it.
IFARM_CLAS12TAGS_BASE = Path('/u/scigroup/cvmfs/geant4/almalinux9-gcc11/clas12Tags')

def check_gemc_version(version, environment, report):
    """Verify that an ifarm clas12Tags version exists before changing GEMC modules.

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

    # A standard active module points GEMC_DATA_DIR at ``BASE/VERSION``. Remove exactly the
    # version component so another requested version is resolved within the same ifarm module
    # family. With no active module data, fall back to the documented shared installation root.
    active_data = environment.get('GEMC_DATA_DIR')
    base = Path(active_data).parent if active_data else IFARM_CLAS12TAGS_BASE
    requested = base / version

    # Check the family root separately from its version child so the transcript distinguishes a
    # missing shared mount from an unavailable GEMC release. Both checks finish before unload.
    report.check('CLAS12TAGS_BASE_DIR', str(base), directory=True)
    report.check('REQUESTED_GEMC_DATA_DIR', str(requested), directory=True)

    # The returned path is not applied here. load_gemc() changes the private environment, and
    # verify_gemc() compares the module-produced GEMC_DATA_DIR with this prechecked expectation.
    return requested

def load_gemc(version, environment, report):
    """Load one resolved GEMC module into the isolated environment inherited by Slurm.

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

    report.text('{START}Switching GEMC version to {END}{INFO}' + version + '{END}{START}...{END}')

    modulecmd = shutil.which('modulecmd', path=environment.get('PATH'))

    if modulecmd is None:
        raise ValueError('modulecmd is unavailable; the selected GEMC module cannot be loaded.')

    # Environment Modules emits executable environment mutations on stdout and user-facing
    # diagnostics on stderr. Capture only stdout; stream stderr directly so its terminal colors
    # survive. The helper applies unload and load sequentially to the same private environment.
    helper = ('import json, os, subprocess, sys\n'
              'for arguments in (("unload", "gemc"), ("load", "gemc/" + sys.argv[2])):\n'
              '    result = subprocess.run([sys.argv[1], "python", *arguments], stdout=subprocess.PIPE, text=True)\n'
              '    if result.returncode:\n'
              '        raise SystemExit(result.returncode)\n'
              '    exec(compile(result.stdout, sys.argv[1], "exec"), {"os": os})\n'
              'print(json.dumps(dict(os.environ)))\n')
    sys.stdout.flush()
    result = subprocess.run([sys.executable, '-c', helper, modulecmd, version], env=environment,
                            stdout=subprocess.PIPE, stderr=sys.stdout, text=True)

    if result.returncode:
        raise ValueError('failed to load GEMC module ' + version + '.')

    try:
        loaded = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise ValueError('GEMC module loading returned an invalid environment.') from error

    if not isinstance(loaded, dict) or not all(isinstance(key, str) and isinstance(value, str)
                                               for key, value in loaded.items()):
        raise ValueError('GEMC module loading returned an invalid environment.')

    environment.clear()
    environment.update(loaded)
    report.text()

def verify_gemc(version, expected_data, environment, report):
    """Verify the module-selected data and executable before any Slurm handoff.

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

    loaded_data_text = environment.get('GEMC_DATA_DIR')

    if not loaded_data_text:
        raise ValueError('loading GEMC ' + version + ' did not set GEMC_DATA_DIR.')

    loaded_data = Path(loaded_data_text).resolve()

    if expected_data is not None and loaded_data != expected_data.resolve():
        raise ValueError(f'loading GEMC {version} selected unexpected GEMC_DATA_DIR: {loaded_data}')

    if loaded_data.name != version:
        raise ValueError(f'loading GEMC {version} selected mismatched GEMC_DATA_DIR: {loaded_data}')

    executable_text = shutil.which('gemc', path=environment.get('PATH'))

    if executable_text is None:
        raise ValueError(f'gemc is unavailable after loading GEMC module {version}.')

    executable = Path(executable_text).resolve()

    if loaded_data != executable and loaded_data not in executable.parents:
        raise ValueError(f'loading GEMC {version} selected an executable outside {loaded_data}: {executable}')

    report.check('SLURM_GEMC_EXECUTABLE', str(executable))
    report.text()

    return executable

def clear_farm(values, root, execute, report, cleared):
    """Handle the optional farm_out log cleanup once for the whole invocation.

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

    # The first two branches make the operation a no-op when disabled or already handled
    # for a previous sample in this invocation.
    if values['CLEAR_FAR_OUT'] == 'false':
        return cleared

    if cleared:
        report.text('farm_out was already cleared for this submission invocation. Preserve newly created job logs.')
    else:
        farm = Path(values['farm_out'])

        # The resolver checks path syntax and existence; repeat the destructive-operation
        # guard here against roots, checkout ancestors, symlinks, and unrelated directories.
        if (not farm.is_dir() or farm.is_symlink() or farm in (Path('/'), Path.home().resolve(), root)
                or farm in root.parents or 'farm_out' not in farm.parts):
            raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

        # Iterate one directory level only. A link to a regular file is left untouched,
        # and preview reports the action without unlinking anything.
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
    """Validate, report, and optionally submit one completed LUND sample.

    Purpose:
        Bridge a resolver-approved sample to the protected GEMC/reconstruction Slurm payload.

    Workflow:
        1. Merge resolver-approved worker values and fixed checkout-owned exports.
        2. Report invocation identity and handle optional invocation-wide farm-log cleanup.
        3. Precheck standard GEMC data, load the selected module, and verify its data/executable.
        4. Apply an optional custom clas12Tags data override and validate detector inputs.
        5. Recheck every selected LUND input and required executable before output replacement.
        6. Preserve outputs in preview or recreate only mchipo/reconhipo during execution.
        7. Report the array contract and call the protected payload through sbatch only in execute.

    Args:
        values: One validated sample dictionary returned by resolve_samples().
        environment: Invocation-owned copy of os.environ. Sample exports and the selected GEMC
            module override inherited values. It is intentionally carried between samples so each
            subsequent module transition starts from the preceding private environment.
        root: Checkout directory containing the protected worker payload.
        execute: False for read-only preview; true for cleanup and Slurm submission.
        report: Shared renderer for the legacy-style transcript.
        farm_cleared: Invocation-wide farm_out cleanup state.

    Returns:
        Updated farm_out state for the next selected sample. This prevents repeated cleanup from
        deleting log files created by an earlier array in the same invocation.

    Failure:
        Missing inputs, unsafe output children, absent commands, or sbatch failure raise and
        stop later samples. All read-only preflight checks occur before mchipo/reconhipo replacement.
        Already accepted Slurm arrays are not canceled when a later sample fails.
    """

    # Copy only worker-facing sample values into this invocation's environment. source and
    # farm_out control Python branches, while RUNNING_DIR and the protected payload path
    # are fixed to the checked-out project.
    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/slurm-submission/external/submit_GEMC_sample.sh'))

    uniform = values['source'] == 'uniform'

    # The preview notice precedes the same input report and validation used by execution.
    if not execute:
        report.text('{INFO}PREVIEW:{END}\nNo sbatch, output replacement or farm_out cleanup; add --execute to submit.')
        report.text()

    report.banner('Slurm submission workflow parameters')

    # Display the inherited checkout plus resolved cleanup and GEMC version so the
    # operator can verify the job environment before any simulation output is replaced.
    for key in ('RUNNING_DIR', 'CLEAR_FAR_OUT', 'GEMC_VERSION'):
        report.value(key, environment[key])

    identity = 'uniform' if uniform else 'physical (' + values['SAMPLE_GENERATOR'] + ')'

    report.value('Sample type', identity, value_color='INFO')

    farm_cleared = clear_farm(values, root, execute, report, farm_cleared)

    report.text()

    # A standard ifarm module selection must have matching shared clas12Tags data. Validate it
    # against the still-active environment before changing modules. An explicit custom checkout
    # is validated below and deliberately bypasses this standard-version directory convention.
    expected_gemc_data = None

    # Resolve standard shared data before altering the module environment. Custom clas12Tags
    # inputs bypass only this standard-directory lookup; they still use a verified GEMC binary.
    if not values['CLAS12TAGS_DIR']:
        expected_gemc_data = check_gemc_version(values['GEMC_VERSION'], environment, report)

    # The module transition runs only after the precheck, and verification proves that both the
    # resulting data path and executable agree with the requested version.
    load_gemc(values['GEMC_VERSION'], environment, report)
    verify_gemc(values['GEMC_VERSION'], expected_gemc_data, environment, report)

    # Module initialization may publish its own settings. Reapply the validated worker contract
    # so explicit sample values and fixed coordinator paths retain final precedence.
    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/slurm-submission/external/submit_GEMC_sample.sh'))

    # A custom clas12Tags checkout is optional. When supplied, it becomes GEMC_DATA_DIR
    # for this and subsequent samples in the invocation after its directory check passes.
    if values['CLAS12TAGS_DIR']:
        report.check('CLAS12TAGS_DIR', values['CLAS12TAGS_DIR'], directory=True)

    # The GEMC environment is preloaded by user ifarm environment. An explicit custom checkout can
    # override GEMC_DATA_DIR, but either source must name an existing directory.
    if values['CLAS12TAGS_DIR']:
        environment['GEMC_DATA_DIR'] = values['CLAS12TAGS_DIR']

    if 'GEMC_DATA_DIR' not in environment:
        raise ValueError('GEMC_DATA_DIR is missing from the preloaded GEMC environment; select --clas12tags-dir for a custom checkout.')

    report.check('GEMC_DATA_DIR', environment['GEMC_DATA_DIR'], directory=True)

    # Both sources report the resolved target, beam, and torus settings here.
    # Array size is reported later with the Slurm job settings.
    report.banner('Sample parameters')

    for key in ('SAMPLE_TARGET_NUCLEUS', 'TARGET_VARIATION', 'BEAM_ENERGY_LABEL',
                'DETECTOR_ENERGY_GROUP', 'TORUS_FIELD'):
        report.value(key, values[key])

    report.text()

    # Keep all sample identity fields together. The field-cage flag affects the physical
    # job label; it does not apply an event-selection cut during submission.
    if uniform:
        report.value('UNIFORM_SAMPLE_CHANNEL', values['UNIFORM_SAMPLE_CHANNEL'], color='INFO')
    else:
        for key in ('GENERATOR_TUNE', 'Q2_CUT', 'FC_STATUS', 'FC_STATUS_ENABLED'):
            report.value(key, values[key], color='INFO')

    report.text()

    # Check detector inputs before simulation-output replacement. Their visible status
    # report follows the output inventory in the Slurm jobs parameters section below.
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

    # Recheck all selected array inputs at handoff time, after resolver validation.
    # A missing or newly emptied LUND file must stop before output replacement.
    run = Path(values['OUTPATH'])

    for index in range(1, int(values['NUM_OF_JOBS']) + 1):
        path = run / 'lundfiles' / f"{values['SAMPLE_FILE_PREFIX']}_{index}.txt"

        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f'missing or empty LUND input: {path}')

    # Preview still verifies the detector executables. Only execution needs sbatch,
    # allowing local inspection of a completed sample without a scheduler command.
    for executable in (('sbatch', 'gemc', 'recon-util') if execute else ('gemc', 'recon-util')):
        if shutil.which(executable, path=environment.get('PATH')) is None:
            raise ValueError(f'{executable} is unavailable in the loaded environment.')

    report.banner('Setting output directories' + (' for ' + values['UNIFORM_SAMPLE_CHANNEL'] if uniform else ''))

    # These are the only simulation output children this coordinator replaces.
    # Reject symlinks before deletion so cleanup cannot follow a redirected child.
    output_dirs = [run / name for name in ('mchipo', 'reconhipo')]

    if any(path.is_symlink() for path in output_dirs):
        raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

    # Execution removes stale simulation products and creates empty directories for the
    # new array. Preview prints that intention and leaves existing products untouched.
    # Neither branch removes or rewrites run/lundfiles.
    if execute:
        report.text('{INFO}Removing old directory structure for MC simulation here...{END}')

        for path in output_dirs:
            if path.is_dir():
                shutil.rmtree(path)
            elif path.exists():
                path.unlink()

        report.text('{INFO}Setting up directory structure for MC simulation here...{END}')

        for path in output_dirs:
            path.mkdir()

        report.text()
    else:
        report.text('{INFO}PREVIEW:{END} would replace mchipo reconhipo under ' + str(run) + '; existing outputs are preserved.')

    report.value('OUTPATH', str(run))
    report.text('{START}Number of files in target directory (OUTPATH):{END}')

    # Retain the shell inventory convention: visible lundfiles entries minus the one
    # monitoring directory. This is a report count, not the selected Slurm array size.
    lund_count = sum(not path.name.startswith('.') for path in (run / 'lundfiles').iterdir()) - 1

    report.value('Number of lund files', lund_count)

    for name, label in (('mchipo', 'Number of mchipo files'), ('reconhipo', 'Number of reconhipo files')):
        directory = run / name
        count = sum(not path.name.startswith('.') for path in directory.iterdir()) if directory.is_dir() else 0

        report.value(label, count)

    report.text()

    # One Slurm array task corresponds to each selected PREFIX_INDEX.txt input. All
    # paths have passed preflight before output replacement; report them in one place.
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

    # Show the exact command in both modes; only the execution branch calls it.
    report.text('{START}Submitted job with command:{END}' if execute else '{INFO}Preview command (not submitted):{END}')
    report.text('{START}sbatch --job-name={END}' + values['SLURM_JOB_NAME'] + '{START} --array={END}' + environment['ARRAY'] + ' ' + payload)

    if execute:
        # Flush the report before handing stdout to sbatch, including redirected logs.
        sys.stdout.flush()

        # Pass argv as a list and use the resolved environment without shell evaluation.
        # A scheduler rejection raises immediately, so no later sample is submitted.
        command = ['sbatch', '--job-name=' + values['SLURM_JOB_NAME'], '--array=' + environment['ARRAY'], payload]

        if subprocess.run(command, env=environment, cwd=root).returncode:
            raise ValueError('sbatch failed; no subsequent sample was submitted.')

    report.text()

    return farm_cleared
# endregion Submission

# Entry point ----------------------------------------------------------------

# region Entry point
def main():
    """Run preview or submission for every selected completed LUND sample.

    Purpose:
        Provide the process boundary called by run.csh after its shell environment is ready.
        Keep Python exceptions, per-sample state, and child environments behind one shell-visible
        integer status without terminating the interactive shell that sourced the launcher.

    Workflow:
        Copy the inherited environment and initialize reporting; parse the CLI; resolve and
        validate every requested sample before processing the first; verify the checkout and
        protected payload boundary; process distinct samples in caller order while carrying the
        private module environment and farm-cleanup state; convert known operational failures into
        one nonzero status.

    Inputs:
        Process arguments and the ifarm environment, including the shared color palette and
        module/reconstruction paths. The current working directory must be the verified checkout
        root because maintained resource paths and the protected payload are checkout-relative.

    Outputs:
        A complete preview or execution transcript on standard output. Execution may create fresh
        mchipo/reconhipo directories and submit arrays; preview performs the same validation without
        those mutations. Neither mode exports its private environment back to the caller's shell.

    Returns:
        Zero when every selected sample is previewed or submitted successfully; one for a
        handled setup, validation, module, filesystem, or scheduler failure.

    Failure:
        A failed sample stops later samples. Slurm arrays accepted earlier in the same
        invocation remain submitted. Errors are printed through Report when available; failures
        before palette construction use a plain standard-error fallback.
    """

    # Keep a plain-text fallback for failures that occur before the shared palette can be
    # validated and a Report can be constructed.
    report = None

    try:
        # Work on an invocation-owned copy so sample exports and GEMC_DATA_DIR changes reach
        # sbatch but do not mutate the interactive shell that launched this Python process.
        environment = dict(os.environ)
        report = Report(environment)

        report.banner('Running Slurm submission script', main=True)
        report.text()

        # Resolve the complete request before processing its first sample. This prevents a
        # later malformed sample from being discovered only after earlier output cleanup.
        args = parser().parse_args()
        root = Path(__file__).resolve().parents[2]
        samples = resolve_samples(args, root)

        # The protected worker must have a path safe for its argument contract, must exist
        # in this checkout, and must be launched from the expected repository directory.
        path_value(root / 'src/slurm-submission/external/submit_GEMC_sample.sh', 'SUBMIT_SCRIPT_FILE')

        if Path.cwd().resolve() != root or not (root / 'src/slurm-submission/external/submit_GEMC_sample.sh').is_file():
            raise ValueError('source the submission script from the CLAS12-sample-generator checkout.')

        # Carry farm_out handling and worker environment through the ordered samples.
        # submit_sample() performs either read-only preview or an explicit Slurm handoff.
        farm_cleared = False

        for values in samples:
            farm_cleared = submit_sample(values, environment, root, args.execute, report, farm_cleared)

        return 0

    except (OSError, ValueError, TypeError, KeyError, RuntimeError) as error:
        # Convert expected operational exceptions to one shell-visible failure status. A
        # RuntimeError with no message means Report.check() already printed the path error.
        if str(error):
            if report:
                report.text('{ERR}Error:{END} ' + str(error))
            else:
                print(f'Error: {error}', file=sys.stderr)

        return 1

# Keep the status from main() as the Python process status for run.csh to capture.
if __name__ == '__main__':
    sys.exit(main())
# endregion Entry point
