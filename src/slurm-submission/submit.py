#!/usr/bin/env python3

#
# Created by Alon Sportes on 21/09/2026.
#

"""Preview or submit existing LUND samples through the protected ifarm payload.

Workflow:
    resolve every input -> report/check preloaded GEMC -> validate worker inputs ->
    prepare mchipo/reconhipo only with --execute -> submit one Slurm array per sample.
    The sourced shell supplies the shared palette and preloaded software environment. Each
    sbatch inherits that environment with resolved sample settings taking precedence. Nothing
    is exported back into the interactive shell. No detector commands are implemented here.

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

    The palette is copied and decoded once; methods write directly to stdout. Banners
    retain the shared shell renderer's 100-column borders and asymmetric title padding.
    """

    def __init__(self, environment):
        """Require all shared colors; never define a second terminal palette."""

        names = ('START', 'ERR', 'COMPLETION', 'INFO', 'WARNING', 'END')

        if any('COLOR_' + name not in environment for name in names):
            raise ValueError('printout colors are unavailable; source the submission workflow through run.csh.')

        self.colors = {name: environment['COLOR_' + name].replace(r'\033', '\033') for name in names}

    def text(self, text=''):
        """Print one line, expanding semantic color markers only."""

        for name, value in self.colors.items():
            text = text.replace('{' + name + '}', value)

        print(text)

    def banner(self, title, main=False):
        """Match code_banner/code_subbanner without requiring shell aliases."""

        padding = 96 - len(title.encode())
        left = max(0, padding // 2)
        right = max(0, padding - padding // 2)
        border, opening, closing = ('/', '//', '//') if main else ('=', '= ', ' =')

        self.text('{START}' + border * 100 + '{END}')
        self.text('{START}' + opening + ' ' * left + '{END}' + title + '{START}' + ' ' * right + closing + '{END}')
        self.text('{START}' + border * 100 + '{END}')

    def value(self, name, value, spaces=1, color='START'):
        """Print an aligned labeled value with the original report spacing."""

        self.text('{' + color + '}' + name + ':{END}' + ' ' * spaces + value)

    def check(self, name, path, directory=False):
        """Print the shell existence check and raise before dependent work on failure."""

        kind = 'directory' if directory else 'file'

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
    """Clear only direct regular farm log files, once per invocation; preview only reports.

    Reject roots, checkout ancestors, home and paths outside a farm_out hierarchy. The
    resolver canonicalizes configured paths; symlink entries inside the directory survive.
    """

    report.banner('Handling farm_out directory clearing and GEMC data')

    if values['CLEAR_FAR_OUT'] == 'false':
        report.text("CLEAR_FAR_OUT$ {START}is set to '{END}false{START}', skipping farm_out directory clearing...{END}")
    elif cleared:
        report.text('farm_out was already cleared for this submission invocation. Preserve newly created job logs.')
    else:
        farm = Path(values['farm_out'])

        if (not farm.is_dir() or farm.is_symlink() or farm in (Path('/'), Path.home().resolve(), root)
                or farm in root.parents or 'farm_out' not in farm.parts):
            raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

        report.banner('Clearing farm_out directory', main=True)

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
    """Report, validate and hand one resolved sample to sbatch; return farm cleanup state.

    environment is invocation-owned and persists custom GEMC_DATA_DIR across samples,
    as the sourced workflow did. Sample overrides replace inherited exports; source and
    farm_out remain coordinator controls. Only validated output children are replaced.
    """

    environment.update({key: value for key, value in values.items() if key not in ('source', 'farm_out')})
    environment.update(RUNNING_DIR=str(root), SLURM_EXPORT_ENV='ALL', SBATCH_EXPORT='ALL',
                       SUBMIT_SCRIPT_FILE=str(root / 'src/slurm-submission/external/submit_GEMC_sample.sh'))

    uniform = values['source'] == 'uniform'

    if not execute:
        report.text('{INFO}PREVIEW:{END}\nNo sbatch, output replacement or farm_out cleanup; add --execute to submit.')
        report.text()

    report.banner('Setup environment variables and paths')

    for key, spaces in (('RUNNING_DIR', 3), ('CLEAR_FAR_OUT', 1), ('GEMC_VERSION', 2)):
        report.value(key, environment[key], spaces)

    identity = '   uniform' if uniform else '  physical (' + values['SAMPLE_GENERATOR'] + ')'

    report.text('{START}Sample type:{INFO}' + identity + '{END}')
    report.text()

    if values['CLAS12TAGS_DIR']:
        report.value('CLAS12TAGS_DIR', values['CLAS12TAGS_DIR'])
        report.check('CLAS12TAGS_DIR', values['CLAS12TAGS_DIR'], directory=True)

    farm_cleared = clear_farm(values, root, execute, report, farm_cleared)

    report.banner('Checking preloaded GEMC data')

    if values['CLAS12TAGS_DIR']:
        environment['GEMC_DATA_DIR'] = values['CLAS12TAGS_DIR']

    if 'GEMC_DATA_DIR' not in environment:
        raise ValueError('GEMC_DATA_DIR is missing from the preloaded GEMC environment; select --clas12tags-dir for a custom checkout.')

    report.value('GEMC_DATA_DIR', environment['GEMC_DATA_DIR'])
    report.check('GEMC_DATA_DIR', environment['GEMC_DATA_DIR'], directory=True)

    report.banner('Uniform sample job parameters' if uniform else values['SAMPLE_GENERATOR'] + ' sample job parameters')

    for key, spaces in (('SAMPLE_TARGET_NUCLEUS', 2), ('TARGET_VARIATION', 7), ('BEAM_ENERGY_LABEL', 6),
                        ('DETECTOR_ENERGY_GROUP', 2), ('TORUS_FIELD', 12), ('NUM_OF_JOBS', 12)):
        report.value(key, values[key], spaces)

    report.text()

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

    run = Path(values['OUTPATH'])

    for index in range(1, int(values['NUM_OF_JOBS']) + 1):
        path = run / 'lundfiles' / f"{values['SAMPLE_FILE_PREFIX']}_{index}.txt"

        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f'missing or empty LUND input: {path}')

    for executable in (('sbatch', 'gemc', 'recon-util') if execute else ('gemc', 'recon-util')):
        if shutil.which(executable, path=environment.get('PATH')) is None:
            raise ValueError(f'{executable} is unavailable in the loaded environment.')

    report.banner('Setting output directories' + (' for ' + values['UNIFORM_SAMPLE_CHANNEL'] if uniform else ''))

    output_dirs = [run / name for name in ('mchipo', 'reconhipo')]

    if any(path.is_symlink() for path in output_dirs):
        raise ValueError('invalid setting or unsafe path; check the submission config or CLI settings.')

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

    # Retain the shell inventory convention: visible entries minus the monitoring directory.
    lund_count = sum(not path.name.startswith('.') for path in (run / 'lundfiles').iterdir()) - 1

    report.text('{START}Number of lund files:     {END} ' + str(lund_count))

    for name, label in (('mchipo', 'Number of mchipo files:   '), ('reconhipo', 'Number of reconhipo files:')):
        directory = run / name
        count = sum(not path.name.startswith('.') for path in directory.iterdir()) if directory.is_dir() else 0

        report.text('{START}' + label + '{END} ' + str(count))

    report.text()

    report.banner('Submitting sbatch job for ' + ('uniform' if uniform else values['SAMPLE_GENERATOR']) + ' sample')

    environment['ARRAY'] = '1-' + values['NUM_OF_JOBS']

    report.value('SLURM_JOB_NAME', values['SLURM_JOB_NAME'])
    report.value('ARRAY', environment['ARRAY'], 10)
    report.text()

    payload = environment['SUBMIT_SCRIPT_FILE']

    report.value('SUBMIT_SCRIPT_FILE', payload)
    report.check('SUBMIT_SCRIPT_FILE', payload)
    report.text()
    report.text('{START}Submitted job with command:{END}' if execute else '{INFO}Preview command (not submitted):{END}')
    report.text('{START}sbatch --job-name={END}' + values['SLURM_JOB_NAME'] + '{START} --array={END}' + environment['ARRAY'] + ' ' + payload)

    if execute:
        sys.stdout.flush()  # Keep scheduler stdout after the printed command, including redirected runs.

        command = ['sbatch', '--job-name=' + values['SLURM_JOB_NAME'], '--array=' + environment['ARRAY'], payload]

        if subprocess.run(command, env=environment, cwd=root).returncode:
            raise ValueError('sbatch failed; no subsequent sample was submitted.')

    report.text()

    return farm_cleared
# endregion Submission

# Entry point ----------------------------------------------------------------

# region Entry point
def main():
    """Resolve all samples, process them in order, and return a sourced-shell-safe status."""

    report = None

    try:
        environment = dict(os.environ)
        report = Report(environment)

        report.banner('Running Slurm submission script', main=True)
        report.text()

        args = parser().parse_args()
        root = Path(__file__).resolve().parents[2]
        samples = resolve_samples(args, root)

        path_value(root / 'src/slurm-submission/external/submit_GEMC_sample.sh', 'SUBMIT_SCRIPT_FILE')

        if Path.cwd().resolve() != root or not (root / 'src/slurm-submission/external/submit_GEMC_sample.sh').is_file():
            raise ValueError('source the submission script from the CLAS12-sample-generator checkout.')

        farm_cleared = False

        for values in samples:
            farm_cleared = submit_sample(values, environment, root, args.execute, report, farm_cleared)

        return 0
    except (OSError, ValueError, TypeError, KeyError, RuntimeError) as error:
        if str(error):
            if report:
                report.text('{ERR}Error:{END} ' + str(error))
            else:
                print(f'Error: {error}', file=sys.stderr)

        return 1

if __name__ == '__main__':
    sys.exit(main())
# endregion Entry point
