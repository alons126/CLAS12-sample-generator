#!/usr/bin/env python3

#
# Created by Alon Sportes on 14/09/2026.
#

"""Validate or locally run the GEMC payload for a completed LUND run.

Purpose:
    Validate exact manifest file counts and delegate detector execution and monitoring to the protected legacy-derived Bash payload.

Workflow:
    Parse -> validate and plan -> preview, or lock and invoke submit_GEMC_sample.sh locally -> record hashes.

    Notes:
    Slurm submission is owned by submit.py, which invokes the protected payload directly with sbatch.
    This module remains the shared validation/local execution implementation.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


# External payload interface ---------------------------------------------------

# region External payload interface
def payload_path(args):
    """Locate the protected Bash payload in a checkout or installed bin directory.

    An explicit --payload overrides discovery. Missing payloads fail before execution.
    """
    supplied = getattr(args, 'payload', None)
    source = Path(__file__).resolve().parents[2] / 'src/slurm-submission/external/submit_GEMC_sample.sh'
    installed = Path(__file__).resolve().with_name('submit_GEMC_sample.sh')
    return Path(supplied or (source if source.is_file() else installed)).resolve(strict=True)


def default_torus(beam_energy):
    """Return the established torus scale for a supported beam energy in GeV."""
    if abs(beam_energy - 2.07052) < 1e-6:
        return 0.5
    if abs(beam_energy - 4.02962) < 1e-6 or abs(beam_energy - 5.98636) < 1e-6:
        return -1.0
    raise ValueError(f'No default torus scale is defined for beam energy {beam_energy}; pass --torus explicitly')


def payload_environment(data, index, path, count, mc, reco, gcard, reconstruction, gemc, recon, args):
    """Supply the original payload's environment and generalized sample labels.

    The manifest's validated per-file count controls both GEMC and reconstruction.
    The preserved script still fixes solenoid -1 and the legacy output layout.
    """
    if args.solenoid != -1 or args.output_naming != 'legacy':
        raise ValueError('Legacy payload requires solenoid -1 and legacy output naming')
    suffix = f'_{index}.txt'
    if not path.name.endswith(suffix) or path.parent != mc.parent.parent / 'lundfiles':
        raise ValueError('Legacy payload requires lundfiles/PREFIX_INDEX.txt matching the manifest index')
    # The original commands leave these paths unquoted; reject shell-splitting/globbing cases.
    for value in (str(mc.parent.parent), path.name, str(gcard), str(reconstruction)):
        if any(ch.isspace() or ch in '*?[]' for ch in value):
            raise ValueError('Legacy payload requires paths without whitespace or glob characters')
    for executable, name in ((gemc, 'gemc'), (recon, 'recon-util')):
        if Path(executable).name != name:
            raise ValueError(f'Legacy payload executable must be named {name}')
    config = data.get('config', {})
    env = dict(os.environ)
    directories = [str(Path(exe).resolve().parent) for exe in (gemc, recon) if '/' in exe]
    env['PATH'] = os.pathsep.join(directories + [env.get('PATH', '')])
    env.update(GCARD_FILE=str(gcard), YAML_FILE=str(reconstruction), TORUS_FIELD=str(args.torus),
               OUTPATH=str(mc.parent.parent), SLURM_ARRAY_TASK_ID=str(index),
               JOB_NEVENTS=str(count),
               SAMPLE_FILE_PREFIX=path.name[:-len(suffix)],
               SAMPLE_GENERATOR=env.get('SAMPLE_GENERATOR', str(config.get('event-generator', data.get('workflow', '')))),
               GENERATOR_TUNE=env.get('GENERATOR_TUNE', env.get('GENIE_TUNE', '')),
               Q2_CUT=env.get('Q2_CUT', str(config.get('q2-cut', ''))),
               SAMPLE_TARGET_NUCLEUS=env.get('SAMPLE_TARGET_NUCLEUS', str(config.get('target', ''))),
               TEMP_BEAM_E=env.get('TEMP_BEAM_E', str(config.get('beam-energy', ''))),
               TEMP_OUTPATH_PARTICLE=env.get('TEMP_OUTPATH_PARTICLE', str(config.get('channel', ''))))
    return env
# endregion


# parser --------------------------------------------------------------------

# region parser
def parser():
    """Define manifest-driven detector-processing options.

    Algorithm:
        Require input manifest, card and reconstruction YAML; resolve the torus scale from manifest
        beam energy when omitted and keep execution opt-in. Solenoid defaults to -1.

    Returns:
        ArgumentParser used by main.
    """

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True,
                   help='completed RUN/lundfiles/lund-gen-monitoring/lund-gen-log.json')
    p.add_argument('--gcard', type=Path, required=True)
    p.add_argument('--reconstruction', type=Path, required=True)
    p.add_argument('--site', type=Path)
    p.add_argument('--torus', type=float, help='torus scale; defaults to 0.5 at 2 GeV and -1 at 4/6 GeV')
    p.add_argument('--solenoid', type=float, default=-1.0)
    p.add_argument('--file-index', type=int, help='One-based manifest index; default: all files')
    p.add_argument('--output-naming', choices=['legacy', 'indexed'], default='legacy')
    p.add_argument('--payload', type=Path, help='External submit_GEMC_sample.sh path; default: checkout or installed copy')
    p.add_argument('--execute', action='store_true', help='Run commands; default only prints them')
    return p
# endregion


# load_plan --------------------------------------------------------------------

# region load_plan
def load_plan(args):
    """Validate inputs and construct GEMC/reconstruction commands.

    Algorithm:
        1. Validate field scales, completed manifest and detector/site files.
        2. Check unique LUND paths, counts and output conflicts.
        3. Check legacy payload compatibility, prepare a command preview and verify counts.

    Args:
        args: Parsed simulation options; execute controls executable availability checks.

    Returns:
        Pair of plan entries (index, commands, mc, reco, record, environment) and site settings; no outputs are created.
    """

    import math
    payload = payload_path(args)

    manifest = args.manifest.resolve(strict=True)
    if (manifest.name != 'lund-gen-log.json' or
            manifest.parent.name != 'lund-gen-monitoring' or
            manifest.parent.parent.name != 'lundfiles'):
        raise ValueError('Expected RUN/lundfiles/lund-gen-monitoring/lund-gen-log.json')
    data = json.loads(manifest.read_text())

    if data.get('schema_version') != 1 or not data.get('files'):
        raise ValueError('Expected a completed schema_version=1 manifest with files')

    if args.torus is None:
        try:
            beam_energy = float(data.get('config', {})['beam-energy'])
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError('Manifest config must contain a numeric beam-energy to default --torus') from error
        args.torus = default_torus(beam_energy)

    if not all(math.isfinite(v) for v in (args.torus, args.solenoid)):
        raise ValueError('Field scales must be finite')

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

    # Manifest paths remain relative to the run directory even though generation diagnostics and the
    # completion log live together below lundfiles/lund-gen-monitoring.
    root = manifest.parent.parent.parent
    plan = []
    seen = set()
    total = 0

    # Validate all manifest entries even when executing only one selected file.
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

        suffix = f'{path.stem}_torus{args.torus}' if args.output_naming == 'legacy' else str(index)
        mc = root / 'mchipo' / f'mc_{suffix}.hipo'
        reco = root / 'reconhipo' / f'recon_{suffix}.hipo'
        record = root / 'simulation' / f'{index}.json'

        if any(p.exists() for p in (mc, reco, record, record.with_suffix(".lock"))):
            raise ValueError(f'Simulation output already exists for file {index}; use a fresh run')

        # Resolve run bookkeeping here; the external Bash payload defines detector commands.
        env = payload_environment(data, index, path, count, mc, reco, gcard, reconstruction, gemc, recon, args)
        # Preview mirrors the external payload command lines; execution uses that script.
        commands = [
            [gemc, '-USE_GUI=0', f'-SCALE_FIELD=binary_torus, {args.torus}',
             '-SCALE_FIELD=binary_solenoid, -1.0', f'-N={count}',
             f'-INPUT_GEN_FILE=lund, {path}', f'-OUTPUT=hipo, {mc}', str(gcard)],
            [recon, '-y', str(reconstruction), '-n', str(count), '-i', str(mc), '-o', str(reco)],
        ]
        plan.append((index, commands, mc, reco, record, env))

    if total != data.get('written_events'):
        raise ValueError('Manifest total does not match per-file event counts')

    return plan, site
# endregion


# main --------------------------------------------------------------------

# region main
def main():
    """Preview or execute the detector-processing plan.

    Algorithm:
        1. Load a validated plan and print each command.
        2. If executing, create an exclusive file lock and invoke the Bash payload.
        3. Bash -e stops on failed commands; the coordinator checks both output files.
        4. Record successful commands and resource hashes; remove successful locks.

    Args:
        No arguments: options come from sys.argv.

    Returns:
        Zero on success; failed execution retains locks and partial outputs for inspection.
    """

    args = parser().parse_args()
    plan, _ = load_plan(args)

    for index, commands, mc, reco, record, env in plan:
        for command in commands:
            print(shlex.join(command), flush=True)

        # Preview is read-only; execution claims outputs before launching external tools.
        if args.execute:
            for directory in (mc.parent, reco.parent, record.parent):
                directory.mkdir(exist_ok=True)

            lock = record.with_suffix('.lock')

            # Retain failed-run locks: no implicit retry over partial HIPO files.

            with lock.open('x'):
                pass

            # The protected legacy-derived script owns GEMC, reconstruction and monitoring.
            payload = payload_path(args)
            subprocess.run(['bash', '-e', str(payload)], env=env, check=True)
            if not mc.is_file() or not mc.stat().st_size or not reco.is_file() or not reco.stat().st_size:
                raise RuntimeError('Legacy payload did not produce both nonempty HIPO outputs')

            record.write_text(json.dumps({
                'commands': commands,
                'payload_sha256': hashlib.sha256(payload.read_bytes()).hexdigest(),
                'gcard_sha256': hashlib.sha256(args.gcard.read_bytes()).hexdigest(),
                'reconstruction_sha256': hashlib.sha256(args.reconstruction.read_bytes()).hexdigest(),
            }, indent=2) + '\n')

            lock.unlink()

    return 0
# endregion


# Command-line entry point ------------------------------------------------

# region Execution
if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, TypeError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
# endregion
