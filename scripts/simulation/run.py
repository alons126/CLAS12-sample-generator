#!/usr/bin/env python3
"""Run GEMC and reconstruction for the exact files/counts in a completed run."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--gcard', type=Path, required=True)
    p.add_argument('--reconstruction', type=Path, required=True)
    p.add_argument('--site', type=Path)
    p.add_argument('--torus', type=float, required=True)
    p.add_argument('--solenoid', type=float, default=-1.0)
    p.add_argument('--file-index', type=int, help='One-based manifest index; default: all files')
    p.add_argument('--execute', action='store_true', help='Run commands; default only prints them')
    return p


def load_plan(args):
    import math
    if not all(math.isfinite(v) for v in (args.torus, args.solenoid)):
        raise ValueError('Field scales must be finite')
    manifest = args.manifest.resolve(strict=True)
    data = json.loads(manifest.read_text())
    if data.get('schema_version') != 1 or not data.get('files'):
        raise ValueError('Expected a completed schema_version=1 manifest with files')
    gcard = args.gcard.resolve(strict=True)
    reconstruction = args.reconstruction.resolve(strict=True)
    site = json.loads(args.site.read_text()) if args.site else {}
    if set(site) - {'gemc', 'recon', 'slurm'}:
        raise ValueError('Unknown site settings')
    gemc, recon = site.get('gemc', 'gemc'), site.get('recon', 'recon-util')
    if not all(isinstance(s, str) and s for s in (gemc, recon)):
        raise ValueError('gemc/recon must be executable names or paths')
    if args.execute:
        for executable in (gemc, recon):
            if not shutil.which(executable):
                raise ValueError(f'Executable not found: {executable}')
    files = data['files']
    if args.file_index is not None and not 1 <= args.file_index <= len(files):
        raise ValueError('file-index outside manifest range')
    root = manifest.parent
    plan = []
    seen = set()
    total = 0
    for index, entry in enumerate(files, 1):
        path = (root / entry['path']).resolve(strict=True)
        if not path.is_relative_to(root) or path in seen or not path.is_file():
            raise ValueError('Manifest files must be unique regular files inside the run directory')
        seen.add(path)
        count = entry['events']
        if type(count) is not int or count <= 0:
            raise ValueError('Invalid manifest event count')
        total += count
        if args.file_index is not None and index != args.file_index:
            continue
        mc = root / 'mchipo' / f'mc_{index}.hipo'
        reco = root / 'reconhipo' / f'recon_{index}.hipo'
        record = root / 'simulation' / f'{index}.json'
        if any(p.exists() for p in (mc, reco, record, record.with_suffix(".lock"))):
            raise ValueError(f'Simulation output already exists for file {index}; use a fresh run')
        commands = [
            [gemc, '-USE_GUI=0', f'-SCALE_FIELD=binary_torus, {args.torus}',
             f'-SCALE_FIELD=binary_solenoid, {args.solenoid}', f'-N={count}',
             f'-INPUT_GEN_FILE=lund, {path}', f'-OUTPUT=hipo, {mc}', str(gcard)],
            [recon, '-y', str(reconstruction), '-n', str(count), '-i', str(mc), '-o', str(reco)],
        ]
        plan.append((index, commands, mc, reco, record))
    if total != data.get('written_events'):
        raise ValueError('Manifest total does not match per-file event counts')
    return plan, site


def main():
    args = parser().parse_args()
    plan, _ = load_plan(args)
    for index, commands, mc, reco, record in plan:
        for command in commands:
            print(shlex.join(command), flush=True)
        if args.execute:
            for directory in (mc.parent, reco.parent, record.parent):
                directory.mkdir(exist_ok=True)
            lock = record.with_suffix('.lock')
            # Retain failed-run locks: no implicit retry over partial HIPO files.
            with lock.open('x'):
                pass
            subprocess.run(commands[0], check=True)
            if not mc.is_file() or not mc.stat().st_size:
                raise RuntimeError(f'GEMC produced no output for file {index}')
            subprocess.run(commands[1], check=True)
            if not reco.is_file() or not reco.stat().st_size:
                raise RuntimeError(f'Reconstruction produced no output for file {index}')
            record.write_text(json.dumps({
                'commands': commands,
                'gcard_sha256': hashlib.sha256(args.gcard.read_bytes()).hexdigest(),
                'reconstruction_sha256': hashlib.sha256(args.reconstruction.read_bytes()).hexdigest(),
            }, indent=2) + '\n')
            lock.unlink()
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, TypeError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
