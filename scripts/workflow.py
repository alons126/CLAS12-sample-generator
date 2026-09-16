#!/usr/bin/env python3

"""Build and launch CLAS12 sample workflows from a local or SSH checkout.

Purpose:
    Centralize launcher settings, optional safe Git updates, CMake builds and workflow dispatch.

Workflow:
    Read settings -> optional pull -> optional build -> optional tests -> run one selected workflow.

Notes:
    Arguments are passed as argv lists; callers select execution explicitly where supported.
"""

import argparse
import json
from pathlib import Path
import shlex
import subprocess
import sys
import os

# Launcher configuration objects -----------------------------------------------
# region Launcher configuration objects
# Purpose: define the checkout anchor and fallback controls shared by shell entry points.
# Lifecycle: settings overlays one selected JSON profile and explicit CLI controls on DEFAULTS.
# These are launcher controls, not particle physics settings. Relative workflow paths use ROOT.
ROOT = Path(__file__).resolve().parents[1]
DEFAULTS = {
    # Workflow and CMake output configuration; names must agree with dispatch below.
    'workflow': 'create-lund', 'source': 'uniform', 'build_dir': 'build/release', 'build_type': 'Release',
    'jobs': 4, 'git_pull': False, 'build': True, 'run': True, 'test': False, # TODO: Remove 'git_pull' from the options. I'll be done in [run.csh](/Users/alon/Projects/CLAS12-sample-generator/run.csh) by default
    # Per-workflow argv lists come from the selected profile; no shell evaluation is performed.
    'arguments': {},
}
# Allowed dispatcher keys, used both by argparse and JSON validation.
WORKFLOWS = ('create-lund', 'submit')
SOURCES = ('uniform', 'physical')
# endregion

COLOR_START = os.environ.get("COLOR_START", "").replace(r"\033", "\033")
COLOR_ERR = os.environ.get("COLOR_ERR", "").replace(r"\033", "\033")
COLOR_COMPLETION = os.environ.get("COLOR_COMPLETION", "").replace(r"\033", "\033")
COLOR_INFO = os.environ.get("COLOR_INFO", "").replace(r"\033", "\033")
COLOR_WARNING = os.environ.get("COLOR_WARNING", "").replace(r"\033", "\033")
COLOR_END = os.environ.get("COLOR_END", "").replace(r"\033", "\033")


# boolean --------------------------------------------------------------------
# region boolean
def boolean(value):
    """Parse a launcher boolean option.

    Algorithm:
        Accept true/false and documented equivalent tokens; reject other input.

    Args:
        value: CLI string to interpret.

    Returns:
        A bool; invalid input raises argparse.ArgumentTypeError.
    """
    
    if value.lower() in ('true', 'yes', 'on', '1'):
        return True
    
    if value.lower() in ('false', 'no', 'off', '0'):
        return False
    
    raise argparse.ArgumentTypeError('Use true or false')
# endregion


# parser --------------------------------------------------------------------
# region parser
def parser():
    """Define launcher options separately from forwarded workflow options.

    Algorithm:
        Register build/update/run controls; leave child arguments for parse_known_args.

    Returns:
        ArgumentParser for the checkout launcher.
    """
    
    p = argparse.ArgumentParser(description=__doc__, epilog='Unrecognized options are forwarded to the selected workflow. Use -- --help for its help.')
    p.add_argument('--run-settings', type=Path, help='JSON settings; defaults to config/run.local.json if present, otherwise config/run.json')
    p.add_argument('--workflow', choices=WORKFLOWS)
    p.add_argument('--source', choices=SOURCES, help='LUND source mode for create-lund')
    p.add_argument('--git-pull', type=boolean)
    p.add_argument('--build', type=boolean)
    p.add_argument('--run', type=boolean)
    p.add_argument('--test', type=boolean)
    p.add_argument('--build-dir')
    p.add_argument('--build-type', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'))
    p.add_argument('--jobs', type=int)
    p.add_argument('--update-only', action='store_true', help=argparse.SUPPRESS)
    
    return p
# endregion


# settings --------------------------------------------------------------------
# region settings
def settings(args):
    """Load and validate the effective launcher profile.

    Algorithm:
        1. Select an explicit JSON file, local profile or checked-in default.
        2. Overlay CLI controls on the defaults and JSON values.
        3. Validate types, workflow names and forwarded argument lists.

    Args:
        args: Parsed launcher options.

    Returns:
        Validated settings dictionary; malformed profiles raise an error.
    """
    
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

    if result['source'] not in SOURCES:
        raise ValueError('Invalid LUND source in run settings')
    
    for key in ('git_pull', 'build', 'run', 'test'):
        if type(result[key]) is not bool:
            raise ValueError(f'{key} must be a JSON boolean')
    
    if type(result['jobs']) is not int or result['jobs'] < 1:
        raise ValueError('jobs must be a positive integer')
    
    if result['build_type'] not in ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'):
        raise ValueError('Invalid build_type')
    
    if not isinstance(result['build_dir'], str) or not result['build_dir']:
        raise ValueError('build_dir must be a nonempty path')
    
    if not isinstance(result['arguments'], dict) or set(result['arguments']) - (set(SOURCES) | {'submit'}):
        raise ValueError('arguments must map workflow names to argument lists')
    
    for name, values in result['arguments'].items():
        if not isinstance(values, list) or not all(isinstance(value, str) for value in values):
            raise ValueError(f'arguments.{name} must be a list of strings')
    
        # Keep forwarding predictable: settings contain --key value pairs or a
        # workflow boolean switch. Never evaluate these values as shell text.
        parse_options(values)
    
    return result
# endregion


# parse_options --------------------------------------------------------------------
# region parse_options
def parse_options(values):
    """Group workflow arguments without evaluating shell text.

    Algorithm:
        Read each --key and optional following value; reject duplicate keys and equals syntax.

    Args:
        values: List of argv strings; negative numbers are valid values.

    Returns:
        List of option groups, each containing a key and optional value.
    """
    
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
# endregion


# forwarded_arguments --------------------------------------------------------------------
# region forwarded_arguments
def forwarded_arguments(defaults, overrides):
    """Merge workflow defaults with explicit CLI options.

    Algorithm:
        Replace default groups with matching CLI keys; pass child --help alone.

    Args:
        defaults: Argument list from the selected JSON workflow.
        overrides: Explicit child options from the launcher command line.

    Returns:
        Flat argv list preserving quoted values as individual strings.
    """
    
    if overrides == ['--help']:
        return overrides
    
    groups = parse_options(overrides)
    replaced = {group[0] for group in groups}
    
    # An explicit --config replaces the default sample profile. Other explicit
    # values override only their corresponding launcher defaults.
    return [word for group in parse_options(defaults) if group[0] not in replaced for word in group] + overrides
# endregion


# execute --------------------------------------------------------------------
# region execute
def execute(command):
    """Run a checked command from the checkout root.

    Algorithm:
        Format the command with option-value pairs on separate lines,
        print it for inspection, then execute its argument list.

    Args:
        command: Executable and arguments; never evaluated as shell source.

    Returns:
        None; subprocess failure propagates to the launcher error handler.
    """

    lines = [shlex.quote(str(command[0]))]

    for arg in command[1:]:
        quoted_arg = shlex.quote(str(arg))

        if str(arg).startswith("-"):
            lines.append(quoted_arg)
        else:
            lines[-1] += f" {quoted_arg}"

    formatted_command = " \\\n    ".join(lines)

    print(f"{COLOR_INFO}Executing command:{COLOR_END}\n{formatted_command}", flush=True)
    print()

    subprocess.run(command, cwd=ROOT, check=True)
    print()
# endregion


# update_repository --------------------------------------------------------------------
# region update_repository
def update_repository():
    """Fast-forward the checkout only when local work is clean.

    Algorithm:
        Check tracked and untracked changes; refuse dirty work; invoke git pull --ff-only.

    Returns:
        None; local edits and divergent history cause failure, without reset or clean.
    """
    
    status = subprocess.run(['git', 'status', '--porcelain', '--untracked-files=normal'], cwd=ROOT, check=True, capture_output=True, text=True)
    
    if status.stdout.strip():
        raise RuntimeError('Checkout has local changes; commit/stash them or use --git-pull false. No files were reset or cleaned.')
    
    execute(['git', 'pull', '--ff-only'])
# endregion


# banner --------------------------------------------------------------------
# region banner
def banner(name):
    # Printers are presentation-only helpers and never determine the exit code.
    """Print a presentation-only workflow status banner.

    Algorithm:
        Try the tcsh printer; fall back to plain text if the shell cannot be started.

    Args:
        name: Printer suffix such as logo, success or stop.

    Returns:
        None; the printer does not determine the workflow result.
    """
    
    try:
        subprocess.run(['tcsh', '-f', str(ROOT / 'scripts/printers' / f'print_{name}.csh')], check=False)
    except OSError:
        print(f'CLAS12 samples: {name}', flush=True)
# endregion


# main --------------------------------------------------------------------
# region main
def main():
    """Run the configured checkout workflow in dependency order.

    Purpose:
        Keep the sourced shell wrappers small while preserving one ordered, checked workflow.

    Algorithm:
        1. Parse launcher options and read settings.
        2. Handle update-only, or perform the optional update and reload settings.
        3. Configure/build and optionally run CTest.
        4. Dispatch one generation, conversion, simulation or submission command.

    Args:
        No arguments: options come from sys.argv.

    Returns:
        Zero after success; the module entry point converts exceptions to failure statuses.
    """
    
    args, forwarded = parser().parse_known_args()
    
    if forwarded[:1] == ['--']:
        forwarded = forwarded[1:]
    
    config = settings(args)
    # banner('logo')
    
    if args.update_only:
        update_repository()
        banner('success')
        return 0
    
    # Update only when requested; the updater refuses local tracked or untracked changes.
    if config['git_pull']:
        update_repository()
        # A pull may change the checked-in run profile.
        config = settings(args)
    
    workflow = config['workflow']
    source = config['source']
    build = Path(config['build_dir'])
    
    if not build.is_absolute():
        build = ROOT / build
    build = build.resolve()
    argument_key = source if workflow == 'create-lund' else workflow
    arguments = forwarded_arguments(config['arguments'].get(argument_key, []), forwarded)
    
    # Compile both sample applications before selecting which workflow to execute.
    if config['build']:
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print(f"{COLOR_START}= Compiling applications                                                                           ={COLOR_END}")
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print()

        # Always let CMake check dependencies, including an uncommitted/replaced
        # targets.h. Git commit stamps cannot detect those edits.
        execute(['cmake', '-S', str(ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE='+config['build_type'],
                 '-DBUILD_UNIFORM=ON', '-DBUILD_GENIE=ON', '-DBUILD_TESTING='+('ON' if config['test'] else 'OFF')])
        execute(['cmake', '--build', str(build), '--parallel', str(config['jobs'])])

        print()
    
    # Tests must succeed before the selected workflow can run.
    if config['test']:
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print(f"{COLOR_START}= Running tests                                                                                    ={COLOR_END}")
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print()

        cache = (build / 'CMakeCache.txt').read_text()
        
        if 'BUILD_TESTING:BOOL=ON' not in cache:
            raise RuntimeError('Tests are not configured; use --build true --test true')
        
        execute(['ctest', '--test-dir', str(build), '--output-on-failure'])
        
        print()
    
    # Dispatch exactly one workflow; generation does not automatically launch GEMC.
    if config['run']:
        if workflow == 'create-lund':
            app = 'clas12-uniform' if source == 'uniform' else 'clas12-generator-to-lund'
            message = f"Creating LUND files from '{COLOR_END}{source}{COLOR_START}' input"
            visible_length = len(f"Creating LUND files from '{source}' input")
            padding = 96 - visible_length

            print(f"{COLOR_START}===================================================================================================={COLOR_END}")
            print(f"{COLOR_START}= {message}{' ' * padding} ={COLOR_END}")
            print(f"{COLOR_START}===================================================================================================={COLOR_END}")
            print()

            executable = build / 'apps' / app

            if not executable.is_file():
                raise RuntimeError(f'Executable missing: {executable}; enable --build true')

            command = [str(executable)]
        else:
            command = [sys.executable, str(ROOT / 'scripts/slurm/submit.py')]

        execute(command + arguments)

    banner('success')

    return 0
# endregion


# Command-line entry point ------------------------------------------------
# region Execution
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
# endregion
