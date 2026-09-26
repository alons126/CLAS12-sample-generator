#
# Created by Alon Sportes on 14/09/2026.
#

#!/usr/bin/env python3

"""Build, test, and start LUND creation for run.csh.

Purpose:
    Keep build settings separate from sample settings and detector submission.

Workflow:
    Parse launcher flags -> load config/run.json -> configure/build -> optional CTest ->
    run uniform-lund-generator or event-generator-to-lund-converter with the original sample arguments.

Inputs:
    A create-lund source, strict build JSON, optional terminal colors, and sample options. Relative
    project paths start at the repository root.

Outputs:
    Build products and completed LUND files. Creation never submits simulation jobs.

Failure:
    Invalid settings or failed commands stop the workflow and return a nonzero status. Child
    arguments are passed directly and are not evaluated by a shell.

Notes:
    run.csh sends submission directly to setup_and_submit.csh. Submission does not use this file or
    the LUND build settings.

CLI options (owned by this launcher):
    --run-settings FILE          Read strict build/test settings (default: config/run.json).
    --workflow create-lund       Select the LUND-creation workflow (required here).
    --source uniform|physical    Select the LUND event source (required for create-lund).
    --build true|false           Configure and build before dispatch (JSON default: true).
    --test true|false            Run CTest after building (JSON default: false).
    --run true|false             Run the selected LUND executable (JSON default: true).
    --build-dir DIRECTORY        Select the CMake binary tree (JSON default: build/release).
    --build-type TYPE            Select Debug, Release, RelWithDebInfo, or MinSizeRel (default: Release).
    --jobs N                     Select positive parallel build workers (JSON default: 4).
    --help                       Print launcher options without updating, building, or running.

Forwarded options:
    Unrecognized arguments are preserved and passed to uniform-lund-generator or
    event-generator-to-lund-converter. Use -- --help after the launcher selections to print that
    executable's authoritative sample options.
"""

import argparse
import json
from pathlib import Path
import shlex
import subprocess
import sys
import os

# Launcher configuration objects ----------------------------------------------------------------------------------------------------------------------------------------

# region Launcher configuration objects
# Purpose:
#     Define the repository root, launcher defaults, accepted names, and optional terminal colors.
#
# Lifecycle:
#     Python creates these values once. Settings copy DEFAULTS before applying JSON and command-line
#     values, so DEFAULTS stays unchanged.
#
# Scope:
#     These values control building, testing, and display. Sample and physics settings stay on the
#     command line or in the selected sample profile.

# Find the repository root from this file so calls work from any directory.
ROOT = Path(__file__).resolve().parents[2]

# Build and test defaults used when JSON and command-line options do not replace them.
DEFAULTS = {
    # The workflow and source remain required command-line choices.
    'build_dir': 'build/release', 'build_type': 'Release',
    'jobs': 4, 'build': True, 'run': True, 'test': False,
}

# Names accepted by the launcher. SOURCES applies only to LUND creation.
WORKFLOWS = ('create-lund', 'submit')
SOURCES = ('uniform', 'physical')

# Read the colors exported by set_colors.csh. Convert written `\033` text to the escape byte used by
# the terminal. Missing values produce plain text.
ERROR_COLOR = os.environ.get("ERROR_COLOR", "").replace(r"\033", "\033")
COMPLETION_COLOR = os.environ.get("COMPLETION_COLOR", "").replace(r"\033", "\033")
SYSTEM_COLOR = os.environ.get("SYSTEM_COLOR", "").replace(r"\033", "\033")
INFO_COLOR = os.environ.get("INFO_COLOR", "").replace(r"\033", "\033")
WARNING_COLOR = os.environ.get("WARNING_COLOR", "").replace(r"\033", "\033")
RESET_COLOR = os.environ.get("RESET_COLOR", "").replace(r"\033", "\033")
# endregion

# Error presentation ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Error presentation
ERROR_PREFIX = f'{ERROR_COLOR}Error:{RESET_COLOR}'

WORKFLOW_GUIDANCE = """Choose one of these forms:
  source run.csh --workflow create-lund --source uniform \\
    --config config/samples/uniform-1e-5986MeV.conf --output OUTPUT_PARENT
  source run.csh --workflow create-lund --source physical \\
    --config config/samples/genie-gst.conf --input 'GST_GLOB' --output OUTPUT_PARENT
  source run.csh --workflow submit --lund-dir RUN/lundfiles
  source run.csh --workflow create-lund --source uniform --build true --test true --run false
Run `source run.csh --help` for launcher options. Add `-- --help` after a selected
create-lund source to see that executable's sample options."""

def error_message(message):
    """Return a message with one colored ``Error:`` prefix.

    Remove any existing prefix before adding the shared form.
    """

    normalized = str(message).replace(f'{ERROR_PREFIX} ', '').replace(ERROR_PREFIX, '').strip()

    return f'{ERROR_PREFIX} {normalized}'

def print_error(message):
    """Write one launcher error to standard error.

    Args:
        message: Text with or without an existing ``Error:`` prefix.

    Outputs:
        Prints the shared prefix and message. Missing colors produce plain text.
    """

    print(error_message(message), file=sys.stderr)

class LauncherArgumentParser(argparse.ArgumentParser):
    """Argument parser that uses the shared launcher error format.

    argparse calls :meth:`error` for invalid choices, values, and option syntax.
    """

    def error(self, message):
        """Print usage and a colored error prefix, then terminate with argparse status 2."""

        self.print_usage(sys.stderr)
        print_error(message)
        self.exit(2)
# endregion

# boolean ---------------------------------------------------------------------------------------------------------------------------------------------------------------

# region boolean
def boolean(value):
    """Convert one command-line token into a launcher boolean.

    Purpose:
        Let build, run, and test options accept clear text values and return a Python bool.

    Workflow:
        Ignore letter case, check the accepted true and false words, and reject anything else.

    Args:
        value: String supplied to an argparse option that uses this function as its ``type``.

    Returns:
        ``True`` for ``true``, ``yes``, ``on``, or ``1``; ``False`` for ``false``, ``no``, ``off``,
        or ``0``. Alphabetic tokens are case-insensitive.

    Assumptions:
        argparse supplies a string. Leading or trailing spaces are invalid.

    Raises:
        argparse.ArgumentTypeError: If the token is not an accepted true or false value.
    """

    # Change letter case only so values with extra spaces still fail.
    if value.lower() in ('true', 'yes', 'on', '1'):
        return True

    if value.lower() in ('false', 'no', 'off', '0'):
        return False

    # argparse sends this error through the shared parser format.
    raise argparse.ArgumentTypeError(error_message('Use true or false'))
# endregion

# parser ----------------------------------------------------------------------------------------------------------------------------------------------------------------

# region parser
def parser():
    """Define launcher options without consuming sample options.

    Purpose:
        Parse build controls here and leave sample options unchanged for the selected program.

    Workflow:
        Register the build profile, workflow, source, build/test switches, and build resources.

    Returns:
        Parser for the options owned by this launcher.

    Failure behavior:
        Invalid values print usage and the shared error format, then exit with status 2.
    """

    # Use this module's documentation for help and point users to the selected program's help.
    p = LauncherArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
                               epilog='Unrecognized options are forwarded unchanged.\n\n' + WORKFLOW_GUIDANCE)

    # Select the workflow and, for LUND creation, the event source.
    p.add_argument('--run-settings', type=Path, help='build/test JSON settings; defaults to config/run.json')
    p.add_argument('--workflow', choices=WORKFLOWS)
    p.add_argument('--source', choices=SOURCES, help='LUND source mode for create-lund')

    # Command-line switches replace matching JSON values.
    p.add_argument('--build', type=boolean)
    p.add_argument('--run', type=boolean)
    p.add_argument('--test', type=boolean)

    # Select the build directory, build type, and worker count. settings() checks them together.
    p.add_argument('--build-dir')
    p.add_argument('--build-type', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'))
    p.add_argument('--jobs', type=int)

    return p
# endregion

# settings --------------------------------------------------------------------------------------------------------------------------------------------------------------

# region settings
def settings(args):
    """Load and check the final launcher settings.

    Purpose:
        Combine defaults, one JSON file, and command-line values before any command runs.

    Workflow:
        Choose the JSON file -> read it from the repository root -> apply defaults, then JSON, then
        command-line values -> add the workflow and source -> check the result.

    Args:
        args: Values returned by ``parser()``. Missing command-line values are ``None``.

    Returns:
        A new checked dictionary. DEFAULTS and the parsed JSON object stay unchanged.

    Assumptions:
        The file is strict JSON and contains only build and test defaults.

    Raises:
        OSError: If the selected profile cannot be read.
        json.JSONDecodeError: If the profile is not valid JSON.
        ValueError: If its structure or values are invalid.
    """

    # Use the requested build profile or the checked-in default.
    path = args.run_settings if args.run_settings is not None else ROOT / 'config/run.json'

    # Resolve relative profile paths from the checkout, not the caller's directory.
    if not path.is_absolute():
        path = ROOT / path

    provided = json.loads(path.read_text())

    # Require one JSON object with only known keys.
    if not isinstance(provided, dict) or set(provided) - DEFAULTS.keys():
        raise ValueError(error_message('Run settings contain unknown keys or are not an object'))

    # Every setting is a single value, so one shallow merge is enough.
    result = {**DEFAULTS, **provided}

    # Copy only command-line values the user supplied.
    for key in DEFAULTS:
        override = getattr(args, key, None)

        if override is not None:
            result[key] = override

    # This driver handles LUND creation. run.csh dispatches submission directly in its shell.
    if args.workflow is None:
        raise ValueError(error_message('Missing required --workflow.\n\n' + WORKFLOW_GUIDANCE))

    result['workflow'] = args.workflow
    result['source'] = args.source

    if result['workflow'] == 'create-lund' and result['source'] is None:
        raise ValueError(error_message('--source uniform|physical is required for create-lund.\n\n' + WORKFLOW_GUIDANCE))

    # Require real JSON booleans; do not accept the integers 0 and 1.
    for key in ('build', 'run', 'test'):
        if type(result[key]) is not bool:
            raise ValueError(error_message(f'{key} must be a JSON boolean'))

    # Require at least one build worker and reject booleans.
    if type(result['jobs']) is not int or result['jobs'] < 1:
        raise ValueError(error_message('jobs must be a positive integer'))

    # Accept only the build types shown by the launcher.
    if result['build_type'] not in ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'):
        raise ValueError(error_message('Invalid build_type'))

    # main() turns this nonempty text into an absolute path.
    if not isinstance(result['build_dir'], str) or not result['build_dir']:
        raise ValueError(error_message('build_dir must be a nonempty path'))

    return result
# endregion

# execute ---------------------------------------------------------------------------------------------------------------------------------------------------------------

# region execute
def execute(command):
    """Run one checked command from the repository root.

    Purpose:
        Print and run every child command in the same safe way.

    Workflow:
        Quote tokens for display -> split long commands across lines -> print the command -> run the
        original argument list from ``ROOT`` and require success.

    Args:
        command: Executable followed by its arguments.

    Returns:
        None after the child exits successfully.

    Outputs:
        Prints the command. Child output remains visible in the same terminal or log.

    Assumptions:
        The list is not empty. Display quoting does not change the arguments sent to the program.

    Raises:
        OSError: If the executable cannot be started.
        subprocess.CalledProcessError: If the child returns a nonzero status. The module entry point
        prints the shared error format and returns the child's effective failure status.
    """

    # Quote a display copy without changing the real arguments.
    lines = [shlex.quote(str(command[0]))]

    for arg in command[1:]:
        quoted_arg = shlex.quote(str(arg))

        # Put each option on a new line and keep its value on that line.
        if str(arg).startswith("-"):
            lines.append(quoted_arg)
        else:
            lines[-1] += f" {quoted_arg}"

    # Show a copyable multiline command. Execution still uses the original argument list.
    formatted_command = " \\\n    ".join(lines)

    # Flush before child output begins.
    print(f"{INFO_COLOR}Executing command:{RESET_COLOR}\n{formatted_command}", flush=True)
    print()

    # Run from the checkout and raise when the command fails.
    subprocess.run(command, cwd=ROOT, check=True)
    print()
# endregion

# banner ----------------------------------------------------------------------------------------------------------------------------------------------------------------

# region banner
def banner(name):
    """Print a workflow status banner.

    Purpose:
        Show the maintained success or stop artwork without changing the workflow result.

    Workflow:
        Build the printer path -> run it with tcsh -> print plain text if tcsh cannot start.

    Args:
        name: Trusted printer suffix such as ``success`` or ``stop``.

    Returns:
        Nothing. Banner failure does not change the workflow status.

    Outputs:
        Prints artwork or a plain-text fallback. It creates no workflow data.

    Failure behavior:
        A printer error is ignored. Failure to start tcsh uses the plain-text fallback.
    """

    # Use an absolute path and pass it as an argument, not shell source text.
    try:
        # Show printer output but ignore its status.
        subprocess.run(['tcsh', '-f', str(ROOT / 'src/launcher/printers' / f'print_{name}.csh')], cwd=ROOT, check=False)
    except OSError:
        # Keep the status visible when tcsh is unavailable.
        print(f'CLAS12 samples: {name}', flush=True)
# endregion

# main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

# region main
def main():
    """Run the requested build, test, and LUND stages in order.

    Purpose:
        Keep shell wrappers focused on checkout and environment setup. This function controls the
        build, test, and LUND program.

    Workflow:
        Parse settings -> resolve the build path -> optionally build -> optionally test -> optionally
        run the selected LUND program -> print success.

    Args:
        None. Launcher and forwarded child options are read from ``sys.argv``.

    Returns:
        Zero when every requested stage succeeds.

    Outputs:
        May update the build tree, run tests, or create LUND files. It does not submit Slurm jobs.

    Raises:
        OSError, ValueError, TypeError, RuntimeError: For invalid configuration or unavailable files.
        subprocess.CalledProcessError: When a checked build, test, generation, or submission command
        fails. The module entry point converts these exceptions to diagnostics and nonzero statuses.
    """

    # Parse launcher options and keep unknown tokens for the selected LUND program.
    args, forwarded = parser().parse_known_args()

    if forwarded[:1] == ['--']:
        forwarded = forwarded[1:]

    if args.workflow == 'submit':
        raise ValueError('Use source run.csh --workflow submit --lund-dir RUN/lundfiles [overrides].')

    # Check all launcher settings before running a command.
    config = settings(args)

    # Resolve relative build paths from the checkout.
    workflow = config['workflow']
    source = config['source']
    build = Path(config['build_dir'])

    if not build.is_absolute():
        build = ROOT / build

    build = build.resolve()

    # Forward sample options unchanged. The launcher adds no hidden sample defaults.
    arguments = forwarded

    # Compile both sample applications before selecting which workflow to execute.
    if config['build']:
        # Color codes do not count toward the visible banner width.
        print(f"{SYSTEM_COLOR}===================================================================================================={RESET_COLOR}")
        print(f"{SYSTEM_COLOR}= Compiling applications                                                                           ={RESET_COLOR}")
        print(f"{SYSTEM_COLOR}===================================================================================================={RESET_COLOR}")
        print()

        # Reconfigure so CMake sees local external-file changes. Build both LUND programs and add test
        # targets only when tests were requested.
        execute(['cmake', '-S', str(ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE='+config['build_type'],
                 '-DBUILD_UNIFORM=ON', '-DBUILD_GENIE=ON', '-DBUILD_TESTING='+('ON' if config['test'] else 'OFF')])

        # Let CMake use its configured build tool and worker count.
        execute(['cmake', '--build', str(build), '--parallel', str(config['jobs'])])

        print()

    # Tests must succeed before the selected workflow can run.
    if config['test']:
        print(f"{SYSTEM_COLOR}===================================================================================================={RESET_COLOR}")
        print(f"{SYSTEM_COLOR}= Running tests                                                                                    ={RESET_COLOR}")
        print(f"{SYSTEM_COLOR}===================================================================================================={RESET_COLOR}")
        print()

        # A reused build directory must already have tests enabled.
        cache = (build / 'CMakeCache.txt').read_text()

        if 'BUILD_TESTING:BOOL=ON' not in cache:
            raise RuntimeError(error_message('Tests are not configured; use --build true --test true'))

        # Show full output for a failed test and stop before LUND creation.
        execute(['ctest', '--test-dir', str(build), '--output-on-failure'])

        print()

    # Dispatch exactly one workflow; generation does not automatically launch GEMC.
    if config['run']:
        if workflow == 'create-lund':
            # Select the uniform generator or the physical-input converter.
            app = 'uniform-lund-generator' if source == 'uniform' else 'event-generator-to-lund-converter'

            # Calculate padding from the visible text without color codes.
            message = f"Creating LUND files from '{RESET_COLOR}{source}{SYSTEM_COLOR}' input"
            visible_length = len(f"Creating LUND files from '{source}' input")
            padding = 96 - visible_length

            print(f"{SYSTEM_COLOR}===================================================================================================={RESET_COLOR}")
            print(f"{SYSTEM_COLOR}= {message}{' ' * padding} ={RESET_COLOR}")
            print(f"{SYSTEM_COLOR}===================================================================================================={RESET_COLOR}")
            print()

            executable = build / 'apps' / app

            # When building is off, report a missing program before starting it.
            if not executable.is_file():
                raise RuntimeError(error_message(f'Executable missing: {executable}; enable --build true'))

            command = [str(executable)]
        else:
            raise ValueError('Submission must be sourced through run.csh --workflow submit.')

        # Append child arguments without shell parsing.
        execute(command + arguments)

    # Every requested stage succeeded.
    banner('success')

    return 0
# endregion

# Command-line entry point ----------------------------------------------------------------------------------------------------------------------------------------------

# region Execution
# Purpose:
#     Return stable process statuses to run.csh while keeping imports free of side effects.
#
# Workflow:
#     Direct execution calls main(). Success returns zero. Interruptions and errors print the stop
#     banner and return a nonzero status.
#
# Outputs:
#     Prints status and error messages. run.csh receives the final process status.
if __name__ == '__main__':
    try:
        # Imports skip this block; direct execution returns main's status.
        sys.exit(main())
    except KeyboardInterrupt:
        # Ctrl-C returns the standard shell status 130.
        banner('stop')
        print_error('Interrupted.')
        sys.exit(130)
    except subprocess.CalledProcessError as error:
        # Keep ordinary exit codes and convert a terminating signal to the shell form 128 + signal.
        banner('stop')

        exit_status = 128-error.returncode if error.returncode < 0 else error.returncode

        # Quote the failed command only for the error message.
        print_error(f'Command failed with exit status {exit_status}: {shlex.join(map(str, error.cmd))}')
        sys.exit(exit_status)
    except (OSError, ValueError, TypeError, RuntimeError) as error:
        # Configuration, filesystem, and launcher errors return status 1.
        banner('stop')
        print_error(error)
        sys.exit(1)
# endregion
