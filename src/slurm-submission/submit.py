#!/usr/bin/env python3

#
# Created by Alon Sportes on 21/09/2026.
#

"""Preview or submit existing LUND samples through the protected ifarm payload.

Purpose:
    Coordinate detector-job submission for already completed uniform or physical LUND runs.
    resolve_inputs.py owns setting validation; the protected shell payload owns GEMC and
    reconstruction commands.

Workflow:
    resolve every input -> report/check preloaded GEMC -> validate worker inputs ->
    prepare mchipo/reconhipo only with --execute -> submit one Slurm array per sample.
    The sourced shell supplies the shared palette and preloaded software environment. Each
    sbatch inherits that environment with resolved sample settings taking precedence. Nothing
    is exported back into the interactive shell. No detector commands are implemented here.

Inputs:
    Completed LUND files and manifests, optional config/CLI overrides, GCARD and YAML files,
    and the ifarm shell environment containing GEMC, Slurm, and the shared COLOR_* palette.

Outputs:
    Preview prints the intended checks and commands without replacing outputs or submitting.
    --execute replaces the selected simulation directories and submits Slurm arrays; it
    preserves lundfiles.

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
    Failures stop subsequent samples, return nonzero and never cancel arrays already accepted
    by Slurm. Invoke through run.csh.
"""

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
        defining ANSI escapes or shell aliases in this module.

    Lifecycle:
        One Report is created from the caller's environment. It owns a decoded copy of the
        six semantic color values; later methods write directly to standard output.

    Output:
        Banners retain the shared shell renderer's 100-column borders and asymmetric title
        padding. Markers such as {START} are expanded only while printing.

    Failure:
        Construction rejects an incomplete color palette. check() prints the missing path
        before raising so the caller can stop without a duplicate diagnostic.
    """

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

        Unknown braces remain ordinary text; an omitted argument prints a blank line.
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
        padding = 96 - len(title.encode())
        left = max(0, padding // 2)
        right = max(0, padding - padding // 2)
        border, opening, closing = ('/', '//', '//') if main else ('=', '= ', ' =')

        self.text('{START}' + border * 100 + '{END}')
        self.text('{START}' + opening + ' ' * left + '{END}' + title + '{START}' + ' ' * right + closing + '{END}')
        self.text('{START}' + border * 100 + '{END}')

    def value(self, name, value, spaces=1, color='START'):
        """Print one colored label and its uncolored value.

        The caller chooses the legacy alignment width and a semantic palette name.
        """

        self.text('{' + color + '}' + name + ':{END}' + ' ' * spaces + value)

    def check(self, name, path, directory=False):
        """Check a required file or directory while preserving the shell transcript.

        Args:
            name: Label printed beside the path status.
            path: Required filesystem path.
            directory: Check for a directory when true, a file otherwise.

        Failure:
            Print the specific missing-path message, then raise RuntimeError so no dependent
            cleanup or Slurm handoff can proceed.
        """

        kind = 'directory' if directory else 'file'

        # The message precedes the check so a failed input is visible in the same sequence
        # as a successful one. RuntimeError has no text because the detail was printed here.
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

    report.banner('Handling farm_out directory clearing and GEMC data')

    # The first two branches make the operation a no-op when disabled or already handled
    # for a previous sample in this invocation.
    if values['CLEAR_FAR_OUT'] == 'false':
        report.text("CLEAR_FAR_OUT$ {START}is set to '{END}false{START}', skipping farm_out directory clearing...{END}")
    elif cleared:
        report.text('farm_out was already cleared for this submission invocation. Preserve newly created job logs.')
    else:
        farm = Path(values['farm_out'])

        # The resolver checks path syntax and existence; repeat the destructive-operation
        # guard here against roots, checkout ancestors, symlinks, and unrelated directories.
        if (not farm.is_dir() or farm.is_symlink() or farm in (Path('/'), Path.home().resolve(), root)
                or farm in root.parents or 'farm_out' not in farm.parts):
            raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

        report.banner('Clearing farm_out directory', main=True)

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
        Merge worker environment values; report sample and detector inputs; check LUND files
        and commands; preview or replace simulation outputs; print the exact Slurm request;
        call sbatch only when execute is true.

    Args:
        values: One validated sample dictionary returned by resolve_samples().
        environment: Invocation-owned copy of os.environ. Sample exports override inherited
            values, and GEMC_DATA_DIR may persist across samples as in the sourced workflow.
        root: Checkout directory containing the protected worker payload.
        execute: False for read-only preview; true for cleanup and Slurm submission.
        report: Shared renderer for the legacy-style transcript.
        farm_cleared: Invocation-wide farm_out cleanup state.

    Returns:
        Updated farm_out state for the next selected sample.

    Failure:
        Missing inputs, unsafe output children, absent commands, or sbatch failure raise and
        stop later samples. Already accepted Slurm arrays are not canceled.
    """

    # Copy only worker-facing sample values into this invocation's environment. source and
    # farm_out control Python branches, while RUNNING_DIR and the protected payload path
    # are fixed to the checked-out project. Slurm must inherit the configured exports.
    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/slurm-submission/external/submit_GEMC_sample.sh'))

    uniform = values['source'] == 'uniform'

    # The preview notice precedes the same input report and validation used by execution.
    if not execute:
        report.text('{INFO}PREVIEW:{END}\nNo sbatch, output replacement or farm_out cleanup; add --execute to submit.')
        report.text()

    report.banner('Setup environment variables and paths')

    # Display the inherited checkout plus resolved cleanup and GEMC version so the
    # operator can verify the job environment before any simulation output is replaced.
    for key, spaces in (('RUNNING_DIR', 3), ('CLEAR_FAR_OUT', 1), ('GEMC_VERSION', 2)):
        report.value(key, environment[key], spaces)

    identity = '   uniform' if uniform else '  physical (' + values['SAMPLE_GENERATOR'] + ')'

    report.text('{START}Sample type:{INFO}' + identity + '{END}')
    report.text()

    # A custom clas12Tags checkout is optional. When supplied, it becomes GEMC_DATA_DIR
    # for this and subsequent samples in the invocation after its directory check passes.
    if values['CLAS12TAGS_DIR']:
        report.value('CLAS12TAGS_DIR', values['CLAS12TAGS_DIR'])
        report.check('CLAS12TAGS_DIR', values['CLAS12TAGS_DIR'], directory=True)

    farm_cleared = clear_farm(values, root, execute, report, farm_cleared)

    # The GEMC environment is preloaded by run.csh. An explicit custom checkout can
    # override GEMC_DATA_DIR, but either source must name an existing directory.
    report.banner('Checking preloaded GEMC data')

    if values['CLAS12TAGS_DIR']:
        environment['GEMC_DATA_DIR'] = values['CLAS12TAGS_DIR']

    if 'GEMC_DATA_DIR' not in environment:
        raise ValueError('GEMC_DATA_DIR is missing from the preloaded GEMC environment; select --clas12tags-dir for a custom checkout.')

    report.value('GEMC_DATA_DIR', environment['GEMC_DATA_DIR'])
    report.check('GEMC_DATA_DIR', environment['GEMC_DATA_DIR'], directory=True)

    # The source-specific banner changes presentation only. Both paths use the same
    # validated target, beam, torus, and array-size values below.
    report.banner('Uniform sample job parameters' if uniform else values['SAMPLE_GENERATOR'] + ' sample job parameters')

    for key, spaces in (('SAMPLE_TARGET_NUCLEUS', 2), ('TARGET_VARIATION', 7), ('BEAM_ENERGY_LABEL', 6),
                        ('DETECTOR_ENERGY_GROUP', 2), ('TORUS_FIELD', 12), ('NUM_OF_JOBS', 12)):
        report.value(key, values[key], spaces)

    report.text()

    # Uniform output is identified by its resolved channel; physical output also reports
    # generator tune, Q2 label, and the legacy field-cage naming flag. The latter is a
    # label, not an event-selection cut performed during submission.
    if uniform:
        report.value('UNIFORM_SAMPLE_CHANNEL', values['UNIFORM_SAMPLE_CHANNEL'], color='INFO')
        report.text()
        report.banner('Setting paths for channel ' + values['UNIFORM_SAMPLE_CHANNEL'])
    else:
        report.banner('Setting paths and parameters')
        report.text()

        for key, spaces in (('SAMPLE_TARGET_NUCLEUS', 1), ('GENERATOR_TUNE', 8), ('Q2_CUT', 16),
                            ('BEAM_ENERGY_LABEL', 5), ('FC_STATUS', 13), ('FC_STATUS_ENABLED', 5)):
            report.value(key, values[key], spaces)

    report.value('OUTPATH', values['OUTPATH'])

    # OUTPATH is the existing run derived from the selected lundfiles directory.
    # The detector-resource folder, GCARD, and YAML must also exist before handoff.
    if not uniform:
        report.text()

    report.check('OUTPATH', values['OUTPATH'], directory=True)

    if not uniform:
        report.check('RUNNING_DIR', str(root), directory=True)

    report.value('REQUIREMENTS_DIR', values['REQUIREMENTS_DIR'])
    report.check('REQUIREMENTS_DIR', values['REQUIREMENTS_DIR'], directory=True)

    for key in ('GCARD_FILE', 'YAML_FILE'):
        report.value(key, values[key])
        report.check(key, values[key])
        report.text()

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
        report.text('{START}Removing old directory structure for MC simulation here...{END}')

        for path in output_dirs:
            if path.is_dir():
                shutil.rmtree(path)
            elif path.exists():
                path.unlink()

        report.text()
        report.text('{START}Setting up directory structure for MC simulation here...{END}')

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

    report.text('{START}Number of lund files:     {END} ' + str(lund_count))

    for name, label in (('mchipo', 'Number of mchipo files:   '), ('reconhipo', 'Number of reconhipo files:')):
        directory = run / name
        count = sum(not path.name.startswith('.') for path in directory.iterdir()) if directory.is_dir() else 0

        report.text('{START}' + label + '{END} ' + str(count))

    report.text()

    # One Slurm array task corresponds to each selected PREFIX_INDEX.txt input. The
    # protected payload receives ARRAY, the job name, and the other resolved exports.
    report.banner('Submitting sbatch job for ' + ('uniform' if uniform else values['SAMPLE_GENERATOR']) + ' sample')

    environment['ARRAY'] = '1-' + values['NUM_OF_JOBS']

    report.value('SLURM_JOB_NAME', values['SLURM_JOB_NAME'])
    report.value('ARRAY', environment['ARRAY'], 10)
    report.text()

    payload = environment['SUBMIT_SCRIPT_FILE']

    report.value('SUBMIT_SCRIPT_FILE', payload)
    report.check('SUBMIT_SCRIPT_FILE', payload)
    report.text()

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

    Workflow:
        Copy the inherited environment and initialize reporting; parse and validate every
        sample; verify the checkout and protected payload; process samples in caller order;
        convert known failures into a nonzero status.

    Inputs:
        Process arguments and the ifarm environment, including the shared color palette and
        preloaded detector software paths.

    Returns:
        Zero when every selected sample is previewed or submitted successfully; one for a
        handled setup, validation, filesystem, or scheduler failure.

    Failure:
        A failed sample stops later samples. Slurm arrays accepted earlier in the same
        invocation remain submitted. Errors are printed through Report when available.
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
