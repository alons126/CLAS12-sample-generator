#!/usr/bin/env python3

#
# Created by Alon Sportes on 14/09/2026.
#

"""Submit one Slurm array task per completed LUND file.

Purpose:
    Implement the second user-facing workflow: submit an ifarm Slurm array that
    runs GEMC and CLAS12 reconstruction for LUND files from a completed manifest.
    This coordinator does not create LUND files and does not run detector
    simulation directly in its own process.

Workflow:
    1. Locate the simulation worker beside this script, unless --runner overrides it.
    2. Reuse the worker's load_plan() function to validate the manifest, detector
       inputs, site configuration, payload, and one-task-per-LUND-file plan.
    3. Validate the site's Slurm resource block.
    4. Build and print one quoted sbatch command whose array index selects a LUND file.
    5. Submit only when the caller explicitly supplies --execute.

Inputs:
    A completed LUND manifest, GCARD, reconstruction YAML, site JSON, magnetic
    field scales, the protected GEMC payload, and optional runner/output settings.

Outputs:
    Preview mode prints the exact sbatch command. Execution mode additionally
    submits that command; each array worker writes GEMC and reconstruction output
    according to the shared simulation plan.

Safety and failure behavior:
    All fixed arguments are assembled as argv lists and shell-quoted before they
    enter sbatch --wrap. Validation errors, filesystem errors, malformed imported
    worker interfaces, and failed sbatch execution produce a colored-neutral
    ``Error:`` message on stderr and exit status 1. Preview is the default.
"""

import argparse
import json
from pathlib import Path
import runpy
import shlex
import subprocess
import sys


# main --------------------------------------------------------------------

# region main
def main():
    """Preview or submit a manifest-sized Slurm array.

    Purpose:
        Keep scheduler construction small and reuse the simulation worker as the
        single authority for manifest and detector-input validation.

    Algorithm:
        1. Resolve the default worker next to this file, preferring the source
           ``run.py`` and falling back to the installed ``clas12-simulate`` name.
        2. Parse command-line inputs and import the worker with runpy.
        3. Call load_plan() in preview mode to validate inputs without running GEMC.
        4. Require the four Slurm resource strings and reject unknown resource keys.
        5. Construct a worker argv list shared by every task and append the Slurm
           array index as the per-task LUND selector.
        6. Print the exact sbatch command, then execute it only with --execute.

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
        Always prints the scheduler command. With --execute, submits one Slurm
        array job; otherwise it performs validation and preview only.
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
    p.add_argument('--torus', type=float, required=True)
    p.add_argument('--solenoid', type=float, default=-1)
    p.add_argument('--payload', type=Path, help='External submit_GEMC_sample.sh path visible to workers')
    p.add_argument('--runner', type=Path, default=default_runner)
    p.add_argument('--output-naming', choices=['legacy', 'indexed'], default='legacy')
    p.add_argument('--execute', action='store_true', help='Submit to Slurm')
    args = p.parse_args()

    # Shared validation and scheduler resources -------------------------------
    # Use the runner's identical manifest/config validation without running GEMC.
    # file_index=None asks for the complete plan so its length becomes the array size.
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

    # Worker command -----------------------------------------------------------
    # Each worker reuses the simulation runner with its own manifest file index.
    # resolve() makes paths independent of the directory from which Slurm starts a task.
    command = ['python3', str(args.runner.resolve()), '--manifest', str(args.manifest.resolve()),
               '--gcard', str(args.gcard.resolve()), '--reconstruction', str(args.reconstruction.resolve()),
               '--site', str(args.site.resolve()), '--output-naming', args.output_naming, '--torus', str(args.torus), '--solenoid', str(args.solenoid), '--execute']

    # payload_path() applies the same default/override rules used during plan validation.
    command += ['--payload', str(module['payload_path'](check))]

    # Quote fixed arguments now, but retain the literal shell variable so Slurm
    # expands a different one-based array index inside each worker environment.
    wrap = shlex.join(command) + ' --file-index "$SLURM_ARRAY_TASK_ID"'

    # Slurm command ------------------------------------------------------------
    # One task is created for every validated manifest file. Site keys already
    # match their long sbatch option names and remain argv elements until --wrap.
    sbatch = ['sbatch', '--nodes=1', '--ntasks=1', '--job-name=clas12-samples', f'--array=1-{len(plan)}']
    sbatch += [f'--{key}={value}' for key, value in slurm.items()]
    sbatch += ['--wrap', wrap]
    print(shlex.join(sbatch), flush=True)

    # Submission remains opt-in after the exact scheduler command is printed.
    if args.execute:
        subprocess.run(sbatch, check=True)

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
