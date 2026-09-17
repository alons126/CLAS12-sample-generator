#!/usr/bin/env python3

#
# Created by Alon Sportes on 14/09/2026.
#

"""Submit one Slurm array task per completed LUND file.

Purpose:
    Reuse simulation validation and send each array worker through the same runner.

Workflow:
    Read site settings -> validate plan -> quote worker command -> preview sbatch or execute it.

Notes:
    Arguments are passed as argv lists; callers select execution explicitly where supported.
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

    Algorithm:
        1. Parse options and reuse simulation planning validation.
        2. Validate required scheduler resource strings.
        3. Quote a worker invocation with a Slurm-expanded file index.
        4. Print sbatch, executing it only with --execute.

    Args:
        No arguments: options come from sys.argv.

    Returns:
        Zero after preview or successful submission; failures reach the module error handler.
    """
    
    here = Path(__file__).resolve()
    default_runner = here.parents[1] / 'simulation' / 'run.py'
    
    if not default_runner.exists():
        default_runner = here.with_name('clas12-simulate')
    
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
    
    # Use the runner's identical manifest/config validation without running GEMC.
    module = runpy.run_path(str(args.runner))
    check = argparse.Namespace(**vars(args))
    check.execute = False
    check.file_index = None
    plan, site = module['load_plan'](check)
    slurm = site.get('slurm', {})
    required = {'account', 'partition', 'time', 'mem'}
    
    if not required.issubset(slurm) or set(slurm) - required - {'output', 'error'} or not all(isinstance(v, str) and v for v in slurm.values()):
        raise ValueError('Site slurm must specify account, partition, time and mem strings')
    
    # Each worker reuses the simulation runner with its own manifest file index.
    command = ['python3', str(args.runner.resolve()), '--manifest', str(args.manifest.resolve()),
               '--gcard', str(args.gcard.resolve()), '--reconstruction', str(args.reconstruction.resolve()),
               '--site', str(args.site.resolve()), '--output-naming', args.output_naming, '--torus', str(args.torus), '--solenoid', str(args.solenoid), '--execute']
    
    command += ['--payload', str(module['payload_path'](check))]
    wrap = shlex.join(command) + ' --file-index "$SLURM_ARRAY_TASK_ID"'
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
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
# endregion
