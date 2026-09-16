#!/usr/bin/env python3

"""Build and launch CLAS12 sample workflows from a local or SSH checkout.

Purpose:
    Centralize launcher settings, CMake builds and workflow dispatch after run.csh synchronizes ifarm.

Workflow:
    Read settings -> optional build -> optional tests -> run one selected workflow.

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
# Purpose:
#     Define the immutable checkout anchor, launcher defaults, accepted dispatch vocabulary, and
#     optional terminal colors used by this process.
# Lifecycle:
#     Python creates these module-level objects once at startup. Configuration loading copies
#     DEFAULTS before overlaying one selected JSON profile and explicit command-line controls; no
#     workflow should mutate DEFAULTS itself.
# Scope:
#     These values control launching and presentation. Physics parameters and target selections
#     remain in the workflow profile and are forwarded through its `arguments` list.

# Absolute repository root derived from this file's stable scripts/ location. All maintained helper,
# build, executable, and profile paths are resolved from this anchor, independent of the caller's
# current working directory.
ROOT = Path(__file__).resolve().parents[1]

# Complete fallback launcher configuration used when a profile or command-line option does not
# override a field. `arguments` maps workflow/source selections to literal argv lists; subprocesses
# receive those lists directly, so their contents are never evaluated by a shell.
DEFAULTS = {
    # Workflow and CMake output configuration; names must agree with dispatch below.
    'workflow': 'create-lund', 'source': 'uniform', 'build_dir': 'build/release', 'build_type': 'Release',
    'jobs': 4, 'build': True, 'run': True, 'test': False,
    # Per-workflow argv lists come from the selected profile; no shell evaluation is performed.
    'arguments': {},
}

# Public workflow and LUND-source vocabularies. argparse uses these tuples for user-facing choices,
# and profile validation uses the same objects so configuration files cannot select an undispatched
# mode. `SOURCES` applies to create-lund; submission consumes LUND output already created earlier.
WORKFLOWS = ('create-lund', 'submit')
SOURCES = ('uniform', 'physical')

# Optional ANSI presentation palette exported by set_colors.csh. The tcsh helper stores escape
# prefixes as the printable sequence `\033`; replacing that prefix here produces the actual control
# character expected by Python's terminal output. Missing variables become empty strings, keeping
# logs readable in noninteractive environments and when workflow.py is invoked without run.csh.
COLOR_START = os.environ.get("COLOR_START", "").replace(r"\033", "\033")
COLOR_ERR = os.environ.get("COLOR_ERR", "").replace(r"\033", "\033")
COLOR_COMPLETION = os.environ.get("COLOR_COMPLETION", "").replace(r"\033", "\033")
COLOR_INFO = os.environ.get("COLOR_INFO", "").replace(r"\033", "\033")
COLOR_WARNING = os.environ.get("COLOR_WARNING", "").replace(r"\033", "\033")
COLOR_END = os.environ.get("COLOR_END", "").replace(r"\033", "\033")
# endregion


# Error presentation ----------------------------------------------------------

# region Error presentation
ERROR_PREFIX = f'{COLOR_ERR}Error:{COLOR_END}'


def error_message(message):
    """Return an error message with exactly one colored ``Error:`` prefix.

    Explicitly raised exceptions use this function when they are created. The normalization also
    handles argparse, which may add option context around an ``ArgumentTypeError`` before displaying
    it, and caught library exceptions whose messages do not yet have the prefix.
    """

    normalized = str(message).replace(f'{ERROR_PREFIX} ', '').replace(ERROR_PREFIX, '').strip()
    return f'{ERROR_PREFIX} {normalized}'


def print_error(message):
    """Write one consistently formatted launcher error to standard error.

    Args:
        message: Human-readable diagnostic, with or without an existing ``Error:`` prefix.

    Outputs:
        Prints ``Error:`` using COLOR_ERR, restores COLOR_END, and then prints the message in the
        terminal's normal color. Missing color environment variables naturally produce plain text.
    """

    print(error_message(message), file=sys.stderr)


class LauncherArgumentParser(argparse.ArgumentParser):
    """Argument parser whose validation failures use the launcher error format.

    argparse calls :meth:`error` for invalid choices, values, and option syntax. Keeping that path on
    the shared formatter ensures parser failures match errors caught by the module entry point.
    """

    def error(self, message):
        """Print usage and a colored error prefix, then terminate with argparse status 2."""

        self.print_usage(sys.stderr)
        print_error(message)
        self.exit(2)
# endregion


# boolean --------------------------------------------------------------------

# region boolean
def boolean(value):
    """Convert one command-line token into a launcher boolean.

    Purpose:
        Let options such as ``--build``, ``--run``, and ``--test`` accept readable shell and JSON
        style values while returning the native bool expected by the workflow dispatcher.

    Workflow:
        Normalize letter case, compare the token with the accepted true and false vocabularies, and
        raise an argparse-specific error when neither vocabulary contains it.

    Args:
        value: String supplied to an argparse option that uses this function as its ``type``.

    Returns:
        ``True`` for ``true``, ``yes``, ``on``, or ``1``; ``False`` for ``false``, ``no``, ``off``,
        or ``0``. Alphabetic tokens are case-insensitive.

    Assumptions:
        argparse supplies a string. Leading or trailing whitespace is considered invalid rather than
        silently removed, which catches malformed profile or shell input.

    Raises:
        argparse.ArgumentTypeError: If the token is outside the accepted vocabularies. argparse then
        passes the diagnostic to ``LauncherArgumentParser.error()``, which applies ``print_error()``
        before stopping the launcher with status 2.
    """

    # Normalize case only: preserving whitespace lets malformed values fail validation explicitly.
    if value.lower() in ('true', 'yes', 'on', '1'):
        return True

    if value.lower() in ('false', 'no', 'off', '0'):
        return False

    # LauncherArgumentParser catches this argparse error path and applies the shared colored prefix.
    raise argparse.ArgumentTypeError(error_message('Use true or false'))
# endregion


# parser --------------------------------------------------------------------

# region parser
def parser():
    """Define launcher options separately from forwarded workflow options.

    Algorithm:
        Register build/run controls; leave child arguments for parse_known_args.

    Returns:
        ArgumentParser for the checkout launcher.
    """
    
    p = LauncherArgumentParser(description=__doc__, epilog='Unrecognized options are forwarded to the selected workflow. Use -- --help for its help.')
    p.add_argument('--run-settings', type=Path, help='JSON settings; defaults to config/run.local.json if present, otherwise config/run.json')
    p.add_argument('--workflow', choices=WORKFLOWS)
    p.add_argument('--source', choices=SOURCES, help='LUND source mode for create-lund')
    p.add_argument('--build', type=boolean)
    p.add_argument('--run', type=boolean)
    p.add_argument('--test', type=boolean)
    p.add_argument('--build-dir')
    p.add_argument('--build-type', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'))
    p.add_argument('--jobs', type=int)
    
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
        raise ValueError(error_message('Run settings contain unknown keys or are not an object'))
    
    result = {**DEFAULTS, **provided}
    
    for key in DEFAULTS:
        override = getattr(args, key, None)
        if override is not None:
            result[key] = override
    
    if result['workflow'] not in WORKFLOWS:
        raise ValueError(error_message('Invalid workflow in run settings'))

    if result['source'] not in SOURCES:
        raise ValueError(error_message('Invalid LUND source in run settings'))
    
    for key in ('build', 'run', 'test'):
        if type(result[key]) is not bool:
            raise ValueError(error_message(f'{key} must be a JSON boolean'))
    
    if type(result['jobs']) is not int or result['jobs'] < 1:
        raise ValueError(error_message('jobs must be a positive integer'))
    
    if result['build_type'] not in ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'):
        raise ValueError(error_message('Invalid build_type'))
    
    if not isinstance(result['build_dir'], str) or not result['build_dir']:
        raise ValueError(error_message('build_dir must be a nonempty path'))
    
    if not isinstance(result['arguments'], dict) or set(result['arguments']) - (set(SOURCES) | {'submit'}):
        raise ValueError(error_message('arguments must map workflow names to argument lists'))
    
    for name, values in result['arguments'].items():
        if not isinstance(values, list) or not all(isinstance(value, str) for value in values):
            raise ValueError(error_message(f'arguments.{name} must be a list of strings'))
    
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
            raise ValueError(error_message(f'Expected --key value or a workflow switch, got {key}'))
    
        if key in seen:
            raise ValueError(error_message(f'Repeated workflow option: {key}'))
    
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
        2. Configure/build and optionally run CTest.
        3. Dispatch LUND creation or Slurm submission.

    Args:
        No arguments: options come from sys.argv.

    Returns:
        Zero after success; the module entry point converts exceptions to failure statuses.
    """
    
    args, forwarded = parser().parse_known_args()
    
    if forwarded[:1] == ['--']:
        forwarded = forwarded[1:]
    
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
            raise RuntimeError(error_message('Tests are not configured; use --build true --test true'))
        
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
                raise RuntimeError(error_message(f'Executable missing: {executable}; enable --build true'))

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
        print_error('Interrupted.')
        sys.exit(130)
    except subprocess.CalledProcessError as error:
        banner('stop')
        exit_status = 128-error.returncode if error.returncode < 0 else error.returncode
        print_error(f'Command failed with exit status {exit_status}: {shlex.join(map(str, error.cmd))}')
        sys.exit(exit_status)
    except (OSError, ValueError, TypeError, RuntimeError) as error:
        banner('stop')
        print_error(error)
        sys.exit(1)
# endregion
