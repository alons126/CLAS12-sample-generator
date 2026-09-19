#!/usr/bin/env python3

#
# Created by Alon Sportes on 14/09/2026.
#

"""Submit one direct Slurm payload job per completed LUND file.

Purpose:
    Implement the second user-facing workflow: submit an ifarm Slurm array that
    runs GEMC and CLAS12 reconstruction for LUND files from a completed manifest.
    This coordinator does not create LUND files and does not run detector
    simulation directly in its own process.

Workflow:
    1. Locate the shared validation and payload-environment implementation beside this script.
    2. Reuse its load_plan() function to validate the manifest, detector inputs,
       site configuration, payload, and one-task-per-LUND-file plan.
    3. Validate the site's Slurm resource block and prepare output directories.
    4. Build and print one direct sbatch command per LUND file, ending with the
       protected submit_GEMC_sample.sh payload.
    5. Submit only when the caller explicitly supplies --execute.

Inputs:
    A completed LUND manifest, GCARD, reconstruction YAML, site JSON, the protected
    GEMC payload, and optional field-scale, runner and output settings. Missing field
    scales use the manifest beam-energy defaults: torus +0.5 at 2 GeV and -1 at 4/6
    GeV; solenoid is always -1 by default.

Outputs:
    Preview mode prints the exact direct sbatch commands. Execution mode additionally
    submits one single-element array job per LUND file; the payload writes GEMC and
    reconstruction output according to the shared simulation plan.

Safety and failure behavior:
    All fixed arguments are assembled as argv lists and shell-quoted for display.
    Validation errors, filesystem errors, malformed imported worker interfaces,
    and failed sbatch execution produce a colored-neutral ``Error:`` message on
    stderr and exit status 1. Preview is the default.
"""

import argparse
import json
from pathlib import Path
import runpy
import shlex
import shutil
import subprocess
import sys


# main --------------------------------------------------------------------

# presentation -------------------------------------------------------------

# region presentation
SEPARATOR = '=' * 100


def banner(title):
    """Print a legacy-style section banner for the submission log."""
    print(SEPARATOR)
    print(f'= {title}')
    print(SEPARATOR)
    print()


def reset_simulation_directory(manifest):
    """Remove prior submission bookkeeping after warning the operator."""
    manifest = Path(manifest).resolve(strict=True)
    if (manifest.name != 'lund-gen-log.json' or
            manifest.parent.name != 'lund-gen-monitoring' or
            manifest.parent.parent.name != 'lundfiles'):
        raise ValueError('Expected RUN/lundfiles/lund-gen-monitoring/lund-gen-log.json')

    simulation = manifest.parent.parent.parent / 'reconhipo' / 'simulation'
    if simulation.exists():
        print(f'WARNING: Removing previous simulation bookkeeping directory: {simulation}', flush=True)
        print('WARNING: Existing locks and local completion records will be discarded.', flush=True)
        shutil.rmtree(simulation)


def print_submission(index, total, environment, command, payload):
    """Print one readable file submission without collapsing it into one long line."""
    export_values = {}
    for item in command:
        if item.startswith('--export=ALL,'):
            for assignment in item[len('--export=ALL,'):].split(','):
                name, value = assignment.split('=', 1)
                export_values[name] = value

    print(f'[{index}/{total}] LUND file: {environment["SAMPLE_FILE_PREFIX"]}_{index}.txt')
    print(f'  Output directory: {environment["OUTPATH"]}')
    print(f'  Events: {environment["JOB_NEVENTS"]}')
    print(f'  Torus: {environment["TORUS_FIELD"]}')
    print(f'  Solenoid: -1')
    print(f'  Payload: {payload}')
    print('  Exported payload settings:')
    for name, value in export_values.items():
        if name == 'PATH':
            value = '<inherited ifarm PATH with configured executable directories>'
        print(f'    {name}: {value}')

    print('  sbatch command:')
    displayed = [item for item in command if not item.startswith('--export=ALL,')]
    print('    ' + ' \\')
    for position, item in enumerate(displayed):
        suffix = ' \\' if position < len(displayed) - 1 else ''
        print(f'      {shlex.quote(item)}{suffix}')
    print()
# endregion

# region main
def main():
    """Preview or submit direct payload jobs for a completed manifest.

    Purpose:
        Keep scheduler construction small and reuse the simulation worker as the
        single authority for manifest and detector-input validation.

    Algorithm:
        1. Resolve the default worker next to this file, preferring the source
           ``run.py`` and falling back to the installed ``clas12-simulate`` name.
        2. Parse command-line inputs and import the worker with runpy.
                3. Call load_plan() in preview mode to validate inputs without running GEMC.
                4. Require the four Slurm resource strings and reject unknown resource keys.
                5. Create one direct sbatch command per plan entry, exporting that file's
                     payload environment and selecting its one-element Slurm array index.
                6. Print every command, then execute them only with --execute.

    Args:
        No arguments: options come from sys.argv.

    Returns:
        int: Zero after a successful preview or submission.

    Raises:
        OSError: If required paths or the runner cannot be read.
        ValueError: If site Slurm resources or shared simulation inputs are invalid.
        KeyError: If the imported runner does not expose the required interface.
        TypeError: If imported configuration values have invalid types.
        subprocess.CalledProcessError: If sbatch returns a nonzero status.

    Side Effects:
        Always prints the scheduler commands. With --execute, submits one direct
        payload job per manifest file; otherwise it performs validation and preview only.
    """

    # Resolve source-tree and installed layouts without requiring a separate mode.
    here = Path(__file__).resolve()
    default_runner = here.with_name('run.py')

    if not default_runner.exists():
        default_runner = here.with_name('clas12-simulate')

    # Command-line inputs ------------------------------------------------------
    # Manifest, detector, and site paths define the simulation plan. Field scales
    # and naming select worker behavior; --execute is the sole submission switch.
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--gcard', type=Path, required=True)
    p.add_argument('--reconstruction', type=Path, required=True)
    p.add_argument('--site', type=Path, required=True)
    p.add_argument('--torus', type=float, help='torus scale; defaults from manifest beam energy')
    p.add_argument('--solenoid', type=float, default=-1)
    p.add_argument('--payload', type=Path, help='External submit_GEMC_sample.sh path visible to workers')
    p.add_argument('--runner', type=Path, default=default_runner)
    p.add_argument('--output-naming', choices=['legacy', 'indexed'], default='legacy')
    p.add_argument('--execute', action='store_true', help='Submit to Slurm')
    args = p.parse_args()

    banner('Preparing direct GEMC and reconstruction submission')
    reset_simulation_directory(args.manifest)

    # Shared validation and scheduler resources -------------------------------
    # Use the runner's identical manifest/config validation without running GEMC.
    # file_index=None asks for the complete validated plan; each entry becomes one
    # direct single-element array submission below.
    module = runpy.run_path(str(args.runner))
    check = argparse.Namespace(**vars(args))
    check.execute = False
    check.file_index = None
    plan, site = module['load_plan'](check)
    slurm = site.get('slurm', {})
    required = {'account', 'partition', 'time', 'mem'}

    # Keep the accepted scheduler contract narrow: four required nonempty strings
    # plus optional stdout/stderr paths that map directly to sbatch options.
    if not required.issubset(slurm) or set(slurm) - required - {'output', 'error'} or not all(isinstance(v, str) and v for v in slurm.values()):
        raise ValueError('Site slurm must specify account, partition, time and mem strings')

    # payload_path() applies the same default/override rules used during plan validation.
    payload = module['payload_path'](check)
    export_names = ('OUTPATH', 'SAMPLE_FILE_PREFIX', 'JOB_NEVENTS', 'GCARD_FILE',
                    'YAML_FILE', 'TORUS_FIELD', 'SAMPLE_GENERATOR', 'GENERATOR_TUNE',
                    'SAMPLE_TARGET_NUCLEUS', 'Q2_CUT', 'TEMP_BEAM_E',
                    'TEMP_OUTPATH_PARTICLE', 'PATH')

    # Prepare one direct payload command per file. A single-element array preserves the
    # manifest index in SLURM_ARRAY_TASK_ID while allowing each file's event count to be
    # exported independently, including a partial final file.
    commands = []
    for index, _, mc, reco, record, environment in plan:
        for directory in (mc.parent, reco.parent, record.parent):
            directory.mkdir(exist_ok=True)

        exports = []
        for name in export_names:
            value = environment.get(name)
            if value is None:
                continue
            if any(character in str(value) for character in (',', '\n', '\r')):
                raise ValueError(f'Payload environment value for {name} contains an unsupported delimiter')
            exports.append(f'{name}={value}')

        prefix = environment['SAMPLE_FILE_PREFIX']
        sbatch = ['sbatch', '--nodes=1', '--ntasks=1', f'--job-name=clas12-{prefix}',
                  f'--array={index}']
        sbatch += [f'--{key}={value}' for key, value in slurm.items()]
        sbatch += [f'--export=ALL,{",".join(exports)}', str(payload)]
        commands.append((index, environment, sbatch))

    banner('Resolved submission plan')
    for index, environment, command in commands:
        print_submission(index, len(commands), environment, command, payload)

    # Submission remains opt-in after every command has been printed and validated.
    if args.execute:
        banner('Submitting jobs with sbatch')
        for _, _, command in commands:
            subprocess.run(command, check=True)

    return 0
# endregion


# Command-line entry point ------------------------------------------------

# region Execution
if __name__ == '__main__':
    # Convert expected operational failures into one stable CLI error contract.
    # Unexpected programming errors retain their traceback during development.
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
# endregion
