#!/usr/bin/env python3
"""Submit one array task per manifest file; default is a dry run."""
import argparse
import json
from pathlib import Path
import runpy
import shlex
import subprocess
import sys


def main():
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
    p.add_argument('--runner', type=Path, default=default_runner)
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
    if set(slurm) != required or not all(isinstance(v, str) and v for v in slurm.values()):
        raise ValueError('Site slurm must specify account, partition, time and mem strings')
    command = ['python3', str(args.runner.resolve()), '--manifest', str(args.manifest.resolve()),
               '--gcard', str(args.gcard.resolve()), '--reconstruction', str(args.reconstruction.resolve()),
               '--site', str(args.site.resolve()), '--torus', str(args.torus), '--solenoid', str(args.solenoid), '--execute']
    wrap = shlex.join(command) + ' --file-index "$SLURM_ARRAY_TASK_ID"'
    sbatch = ['sbatch', '--nodes=1', '--ntasks=1', '--job-name=clas12-samples', f'--array=1-{len(plan)}']
    sbatch += [f'--{key}={value}' for key, value in slurm.items()]
    sbatch += ['--wrap', wrap]
    print(shlex.join(sbatch), flush=True)
    if args.execute:
        subprocess.run(sbatch, check=True)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
