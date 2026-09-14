#!/usr/bin/env python3
"""Build and launch CLAS12 sample workflows from a local or SSH checkout."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULTS = {
    'workflow': 'uniform', 'build_dir': 'build/release', 'build_type': 'Release',
    'jobs': 4, 'git_pull': False, 'build': True, 'run': True, 'test': False,
    'arguments': {},
}
WORKFLOWS = ('uniform', 'genie', 'simulate', 'submit')


def boolean(value):
    if value.lower() in ('true', 'yes', 'on', '1'):
        return True
    if value.lower() in ('false', 'no', 'off', '0'):
        return False
    raise argparse.ArgumentTypeError('Use true or false')


def parser():
    p = argparse.ArgumentParser(description=__doc__, epilog='Unrecognized options are forwarded to the selected workflow. Use -- --help for its help.')
    p.add_argument('--run-settings', type=Path, help='JSON settings; defaults to config/run.local.json if present, otherwise config/run.json')
    p.add_argument('--workflow', choices=WORKFLOWS)
    p.add_argument('--git-pull', type=boolean)
    p.add_argument('--build', type=boolean)
    p.add_argument('--run', type=boolean)
    p.add_argument('--test', type=boolean)
    p.add_argument('--build-dir')
    p.add_argument('--build-type', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'))
    p.add_argument('--jobs', type=int)
    p.add_argument('--update-only', action='store_true', help=argparse.SUPPRESS)
    return p


def settings(args):
    path = args.run_settings
    if path is None:
        local = ROOT / 'config/run.local.json'
        path = local if local.is_file() else ROOT / 'config/run.json'
    if not path.is_absolute():
        path = ROOT / path
    provided = json.loads(path.read_text())
    if not isinstance(provided, dict) or set(provided) - DEFAULTS.keys():
        raise ValueError('Run settings contain unknown keys or are not an object')
    result = {**DEFAULTS, **provided}
    for key in DEFAULTS:
        override = getattr(args, key, None)
        if override is not None:
            result[key] = override
    if result['workflow'] not in WORKFLOWS:
        raise ValueError('Invalid workflow in run settings')
    for key in ('git_pull', 'build', 'run', 'test'):
        if type(result[key]) is not bool:
            raise ValueError(f'{key} must be a JSON boolean')
    if type(result['jobs']) is not int or result['jobs'] < 1:
        raise ValueError('jobs must be a positive integer')
    if result['build_type'] not in ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'):
        raise ValueError('Invalid build_type')
    if not isinstance(result['build_dir'], str) or not result['build_dir']:
        raise ValueError('build_dir must be a nonempty path')
    if not isinstance(result['arguments'], dict) or set(result['arguments']) - set(WORKFLOWS):
        raise ValueError('arguments must map workflow names to argument lists')
    for name, values in result['arguments'].items():
        if not isinstance(values, list) or not all(isinstance(value, str) for value in values):
            raise ValueError(f'arguments.{name} must be a list of strings')
        # Keep forwarding predictable: settings contain --key value pairs or a
        # workflow boolean switch. Never evaluate these values as shell text.
        parse_options(values)
    return result


def parse_options(values):
    options = []
    seen = set()
    i = 0
    while i < len(values):
        key = values[i]
        if not key.startswith('--') or key == '--' or '=' in key:
            raise ValueError(f'Expected --key value or a workflow switch, got {key}')
        if key in seen:
            raise ValueError(f'Repeated workflow option: {key}')
        seen.add(key)
        group = [key]
        if i+1 < len(values) and not values[i+1].startswith('--'):
            i += 1
            group.append(values[i])
        options.append(group)
        i += 1
    return options


def forwarded_arguments(defaults, overrides):
    if overrides == ['--help']:
        return overrides
    groups = parse_options(overrides)
    replaced = {group[0] for group in groups}
    # An explicit --config replaces the default sample profile. Other explicit
    # values override only their corresponding launcher defaults.
    return [word for group in parse_options(defaults) if group[0] not in replaced for word in group] + overrides


def execute(command):
    print('+ ' + shlex.join([str(arg) for arg in command]), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def update_repository():
    status = subprocess.run(['git', 'status', '--porcelain', '--untracked-files=normal'], cwd=ROOT, check=True, capture_output=True, text=True)
    if status.stdout.strip():
        raise RuntimeError('Checkout has local changes; commit/stash them or use --git-pull false. No files were reset or cleaned.')
    execute(['git', 'pull', '--ff-only'])


def banner(name):
    # Printers are presentation-only helpers and never determine the exit code.
    try:
        subprocess.run(['tcsh', '-f', str(ROOT / 'scripts/printers' / f'print_{name}.csh')], check=False)
    except OSError:
        print(f'CLAS12 samples: {name}', flush=True)


def main():
    args, forwarded = parser().parse_known_args()
    if forwarded[:1] == ['--']:
        forwarded = forwarded[1:]
    config = settings(args)
    banner('logo')
    if args.update_only:
        update_repository()
        banner('success')
        return 0
    if config['git_pull']:
        update_repository()
        # A pull may change the checked-in run profile.
        config = settings(args)
    workflow = config['workflow']
    build = Path(config['build_dir'])
    if not build.is_absolute():
        build = ROOT / build
    build = build.resolve()
    arguments = forwarded_arguments(config['arguments'].get(workflow, []), forwarded)
    if config['build']:
        # Always let CMake check dependencies, including an uncommitted/replaced
        # targets.h. Git commit stamps cannot detect those edits.
        execute(['cmake', '-S', str(ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE='+config['build_type'],
                 '-DBUILD_UNIFORM=ON', '-DBUILD_GENIE=ON', '-DBUILD_TESTING='+('ON' if config['test'] else 'OFF')])
        execute(['cmake', '--build', str(build), '--parallel', str(config['jobs'])])
    if config['test']:
        cache = (build / 'CMakeCache.txt').read_text()
        if 'BUILD_TESTING:BOOL=ON' not in cache:
            raise RuntimeError('Tests are not configured; use --build true --test true')
        execute(['ctest', '--test-dir', str(build), '--output-on-failure'])
    if config['run']:
        if workflow in ('uniform', 'genie'):
            executable = build / 'apps' / ('clas12-uniform' if workflow == 'uniform' else 'clas12-genie-to-lund')
            if not executable.is_file():
                raise RuntimeError(f'Executable missing: {executable}; enable --build true')
            command = [str(executable)]
        else:
            script = 'scripts/simulation/run.py' if workflow == 'simulate' else 'scripts/slurm/submit.py'
            command = [sys.executable, str(ROOT / script)]
        execute(command + arguments)
    banner('success')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        banner('stop')
        print('Interrupted.', file=sys.stderr)
        sys.exit(130)
    except subprocess.CalledProcessError as error:
        banner('stop')
        sys.exit(128-error.returncode if error.returncode < 0 else error.returncode)
    except (OSError, ValueError, TypeError, RuntimeError) as error:
        banner('stop')
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
