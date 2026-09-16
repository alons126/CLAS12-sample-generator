#!/usr/bin/env python3

"""Configure, build, and dispatch the two CLAS12 sample-generator workflows.

Purpose:
    Provide one maintained Python entry point behind ``run.csh``. The module resolves launcher
    configuration, prepares the CMake applications, optionally validates them, and then starts either
    LUND creation or ifarm simulation-job submission.

Workflow:
    1. Parse launcher-owned options while retaining child-workflow options for later forwarding.
    2. Load ``config/run.local.json`` when present, otherwise ``config/run.json``, and apply explicit
       command-line overrides.
    3. Optionally configure and build the maintained C++ applications.
    4. Optionally run CTest against that build.
    5. Dispatch exactly one user-facing workflow:
       - ``create-lund --source uniform`` generates configured random acceptance samples;
       - ``create-lund --source physical`` converts supported event-generator truth into LUND; or
       - ``submit`` sends existing LUND, GCARD, and YAML inputs to ifarm Slurm jobs.

Inputs:
    Launcher arguments, one strict JSON run profile, optional terminal-color environment variables,
    and the selected child workflow's own files and forwarded options.

Outputs:
    CMake build products when building is enabled, plus the outputs owned by the selected child:
    completed LUND files for creation or submitted Slurm jobs for simulation. Creation and submission
    remain separate; finishing LUND creation never submits GEMC automatically.

Execution context:
    ``run.csh`` normally invokes this module after intentionally synchronizing the disposable ifarm
    checkout and sourcing the server environment. Direct local invocation is useful for building,
    testing, and previewing configuration; GEMC and reconstruction execute through ifarm submission.

Failure behavior:
    Invalid launcher/profile input and failed checked subprocesses stop later stages, print the stop
    banner and a colored ``Error:`` diagnostic, and return a nonzero status to ``run.csh``.

Notes:
    Commands are passed to subprocesses as argv lists without shell evaluation. Generator-specific
    physics options remain owned by the selected executable rather than this launcher.
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


# Error presentation ----------------------------------------------------------------------------------------------------------------------------------------------------

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


# boolean ---------------------------------------------------------------------------------------------------------------------------------------------------------------

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


# parser ----------------------------------------------------------------------------------------------------------------------------------------------------------------

# region parser
def parser():
    """Define launcher options separately from forwarded workflow options.

    Purpose:
        Describe only the controls owned by this Python launcher. Options that configure uniform
        generation, physical conversion, or ifarm submission remain unknown here and are forwarded
        unchanged to the selected child workflow.

    Workflow:
        Register profile selection, workflow dispatch, build/test controls, and build-resource
        settings. ``main()`` later calls ``parse_known_args()`` so these known options become the
        launcher namespace while the remaining argv tokens become child-workflow overrides.

    Returns:
        A ``LauncherArgumentParser`` configured with the launcher-owned option vocabulary.

    Failure behavior:
        Invalid choices, integers, and boolean tokens flow through ``LauncherArgumentParser.error()``,
        which prints usage followed by the shared colored error format and exits with status 2.
    """

    # Use the module documentation as the help description. The epilog tells users how to request
    # help from a selected child executable instead of stopping at this launcher's own help screen.
    p = LauncherArgumentParser(description=__doc__, epilog='Unrecognized options are forwarded to the selected workflow. Use -- --help for its help.')

    # Profile and dispatch controls select one of the project's two user-facing workflows and, for
    # create-lund, whether its event content comes from uniform sampling or physical generator data.
    p.add_argument('--run-settings', type=Path, help='JSON settings; defaults to config/run.local.json if present, otherwise config/run.json')
    p.add_argument('--workflow', choices=WORKFLOWS)
    p.add_argument('--source', choices=SOURCES, help='LUND source mode for create-lund')

    # Execution-stage switches override matching JSON booleans. The custom converter accepts explicit
    # true/false values so an ifarm command can enable or disable each stage without editing a profile.
    p.add_argument('--build', type=boolean)
    p.add_argument('--run', type=boolean)
    p.add_argument('--test', type=boolean)

    # Build controls choose the CMake output tree, configuration, and parallel build-worker count.
    # Their cross-field constraints, including a positive jobs value, are checked in settings().
    p.add_argument('--build-dir')
    p.add_argument('--build-type', choices=('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'))
    p.add_argument('--jobs', type=int)

    return p
# endregion


# settings --------------------------------------------------------------------------------------------------------------------------------------------------------------

# region settings
def settings(args):
    """Load and validate the effective launcher profile.

    Purpose:
        Resolve the launcher's defaults, one JSON run profile, and explicit launcher options into a
        single trusted configuration before any build, test, generation, or submission command runs.

    Workflow:
        1. Use ``--run-settings`` when supplied; otherwise prefer the untracked local profile and fall
           back to the checked-in profile.
        2. Resolve relative profile paths from the repository root and parse the JSON object.
        3. Apply values in increasing precedence: ``DEFAULTS`` < JSON profile < explicit CLI options.
        4. Validate dispatcher names, build controls, and child-workflow argument lists.

    Args:
        args: Launcher namespace returned by ``parser()``. Attributes left unspecified by the user
            are ``None`` and therefore do not replace JSON or built-in values.

    Returns:
        A new validated dictionary containing every key defined by ``DEFAULTS``. The module-level
        defaults and the parsed JSON object are not mutated.

    Assumptions:
        The profile is strict JSON. Its ``arguments`` values are literal argv arrays, not shell text,
        and their keys are limited to ``uniform``, ``physical``, and ``submit``.

    Raises:
        OSError: If the selected profile cannot be read.
        json.JSONDecodeError: If the profile is not valid JSON.
        ValueError: If its shape, values, or forwarded option syntax violates the launcher contract.
        All diagnostics are given the shared colored ``Error:`` prefix before reaching the user.
    """

    # An explicit profile always wins. Without one, allow a developer- or server-specific local file
    # to override the versioned default without requiring edits to config/run.json.
    path = args.run_settings

    if path is None:
        local = ROOT / 'config/run.local.json'
        path = local if local.is_file() else ROOT / 'config/run.json'

    # Interpret relative paths from the checkout rather than the directory from which run.csh or this
    # module was invoked. read_text and json.loads deliberately propagate I/O and syntax failures.
    if not path.is_absolute():
        path = ROOT / path

    provided = json.loads(path.read_text())

    # Reject arrays, scalar JSON values, and misspelled/unsupported top-level keys before merging;
    # silently accepting an unknown key could make a requested control appear to take effect.
    if not isinstance(provided, dict) or set(provided) - DEFAULTS.keys():
        raise ValueError(error_message('Run settings contain unknown keys or are not an object'))

    # This is a shallow overlay by design. In particular, a profile-provided `arguments` object is
    # the complete per-workflow argument mapping for that profile rather than a partial nested merge.
    result = {**DEFAULTS, **provided}

    # argparse stores omitted launcher controls as None. Copy only explicit values so command-line
    # options have final precedence without erasing fields selected by the JSON profile.
    for key in DEFAULTS:
        override = getattr(args, key, None)
        if override is not None:
            result[key] = override

    # Validate the two dispatch axes against the same constants used to build argparse choices.
    if result['workflow'] not in WORKFLOWS:
        raise ValueError(error_message('Invalid workflow in run settings'))

    if result['source'] not in SOURCES:
        raise ValueError(error_message('Invalid LUND source in run settings'))

    # Require real JSON booleans. Python considers bool a subclass of int, so exact type checks keep
    # values such as 0 and 1 from silently acting as false and true in a profile.
    for key in ('build', 'run', 'test'):
        if type(result[key]) is not bool:
            raise ValueError(error_message(f'{key} must be a JSON boolean'))

    # Build parallelism must be an integer of at least one; booleans are rejected by the exact check.
    if type(result['jobs']) is not int or result['jobs'] < 1:
        raise ValueError(error_message('jobs must be a positive integer'))

    # Limit configurations to the CMake build types supported by the launcher interface.
    if result['build_type'] not in ('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'):
        raise ValueError(error_message('Invalid build_type'))

    # Keep this value as a nonempty string here; main() later resolves it to an absolute Path.
    if not isinstance(result['build_dir'], str) or not result['build_dir']:
        raise ValueError(error_message('build_dir must be a nonempty path'))

    # Each key supplies defaults for one create-lund source or for the submit workflow. Additional
    # keys would have no valid dispatch target and are rejected rather than ignored.
    if not isinstance(result['arguments'], dict) or set(result['arguments']) - (set(SOURCES) | {'submit'}):
        raise ValueError(error_message('arguments must map workflow names to argument lists'))

    for name, values in result['arguments'].items():
        # Preserve argv boundaries exactly: every item must already be a string, including numeric
        # values that will be interpreted later by the selected executable.
        if not isinstance(values, list) or not all(isinstance(value, str) for value in values):
            raise ValueError(error_message(f'arguments.{name} must be a list of strings'))

        # Keep forwarding predictable: settings contain --key value pairs or a
        # workflow boolean switch. Never evaluate these values as shell text.
        parse_options(values)

    return result
# endregion


# parse_options ---------------------------------------------------------------------------------------------------------------------------------------------------------

# region parse_options
def parse_options(values):
    """Group workflow arguments without evaluating shell text.

    Purpose:
        Convert a flat child-workflow argv list into option groups that can be validated and merged by
        option name. This lets explicit command-line options replace matching profile defaults without
        parsing, expanding, or executing their values as shell source.

    Workflow:
        1. Read a token that must use the ``--key`` form.
        2. Reject the separator token, ``--key=value`` syntax, and repeated option names.
        3. Attach the next token as the option value unless it begins another ``--key`` group.
        4. Append the one- or two-token group and continue until the input is consumed.

    Args:
        values: Ordered list of argument strings from a JSON profile or explicit child-workflow
            overrides. Negative numbers such as ``-1`` are values because only ``--`` begins a new
            option group.

    Returns:
        A new ordered list of groups. Each group contains one ``--key`` and, when present, its single
        following value. The input list and string contents are unchanged.

    Assumptions:
        Child interfaces use long options with zero or one following value. Positional arguments,
        multiple values after one key, a bare ``--`` separator, and equals syntax are outside this
        launcher's profile contract. Whether a key is recognized belongs to the selected child.

    Raises:
        ValueError: If a group does not begin with a valid long-option token or if an option appears
        more than once. The exception includes the shared colored ``Error:`` prefix.
    """

    # Preserve group order for predictable command printouts and execution. `seen` enforces one value
    # source per option within each list, while the index allows a consumed value token to be skipped.
    options = []
    seen = set()
    i = 0

    while i < len(values):
        key = values[i]

        # Accept only the separate `--key [value]` representation. A bare separator has no child argv
        # on either side inside a stored profile, and equals syntax would complicate key replacement.
        if not key.startswith('--') or key == '--' or '=' in key:
            raise ValueError(error_message(f'Expected --key value or a workflow switch, got {key}'))

        # Repetition within one source is ambiguous. Cross-source replacement is handled later by
        # forwarded_arguments(), after defaults and explicit overrides have been parsed separately.
        if key in seen:
            raise ValueError(error_message(f'Repeated workflow option: {key}'))

        seen.add(key)
        group = [key]

        # Any following token that does not start with `--` is the single value. This intentionally
        # accepts negative numeric strings and paths beginning with one dash.
        if i+1 < len(values) and not values[i+1].startswith('--'):
            i += 1
            group.append(values[i])

        options.append(group)
        i += 1

    return options
# endregion


# forwarded_arguments ---------------------------------------------------------------------------------------------------------------------------------------------------

# region forwarded_arguments
def forwarded_arguments(defaults, overrides):
    """Merge workflow defaults with explicit CLI options.

    Purpose:
        Combine the selected run profile's child-workflow arguments with options written after the
        launcher arguments, giving the user's explicit command line final precedence by option name.

    Workflow:
        1. If the only override is ``--help``, return it alone so the child can display help without
           receiving profile-specific required inputs or starting normal work.
        2. Parse explicit overrides into ``--key [value]`` groups and collect their keys.
        3. Parse profile defaults, discard groups whose keys are explicitly replaced, flatten the
           retained groups, and append the original overrides in their supplied order.

    Args:
        defaults: Literal argv list from the JSON profile's selected ``arguments`` entry.
        overrides: Literal child-workflow argv tokens left by ``parse_known_args()``. A leading bare
            ``--`` has already been removed by ``main()``.

    Returns:
        A new flat argv list containing unreplaced profile groups followed by explicit overrides.
        Token boundaries and value text are preserved, and neither input list is mutated.

    Assumptions:
        Both inputs follow the grammar enforced by ``parse_options()``. Replacement compares exact
        long-option names; this function does not interpret aliases or inspect option values.

    Raises:
        ValueError: If either list contains invalid syntax or repeats a key within that list. The
        exception carries the shared colored ``Error:`` prefix from ``parse_options()``.
    """

    # Child help is a complete command in its own right. Profile arguments could hide the requested
    # help behind missing-file checks or otherwise make the displayed interface profile-dependent.
    if overrides == ['--help']:
        return overrides

    # Parsing validates the override grammar and supplies the exact keys that take precedence.
    groups = parse_options(overrides)
    replaced = {group[0] for group in groups}

    # `--config` therefore replaces the default sample profile, while every other explicit key uses
    # the same predictable replacement rule. Keeping overrides last also makes the printed command
    # reflect their final precedence clearly.
    return [word for group in parse_options(defaults) if group[0] not in replaced for word in group] + overrides
# endregion


# execute ---------------------------------------------------------------------------------------------------------------------------------------------------------------

# region execute
def execute(command):
    """Run a checked command from the checkout root.

    Purpose:
        Provide one visible, checked subprocess boundary for CMake, CTest, LUND applications, and the
        maintained submission coordinator. Every launched command therefore uses the same working
        directory, logging format, argv safety, and failure propagation.

    Workflow:
        1. Shell-quote each token for display only.
        2. Start a display line for the executable and for each option-like token; attach ordinary
           values to the preceding line so long commands remain readable in terminal and Slurm logs.
        3. Print and flush the reconstructed command before child output begins.
        4. Execute the original argv sequence from ``ROOT`` with ``check=True``.

    Args:
        command: Nonempty sequence containing an executable followed by its argv tokens. Elements may
            be strings or path-like objects accepted by ``subprocess.run``.

    Returns:
        None after the child exits successfully.

    Outputs:
        Writes the readable command and spacing to standard output. The child inherits this process's
        standard input, output, and error streams, so its normal monitoring remains visible.

    Assumptions:
        The caller supplies a nonempty argv sequence. Display quoting is informational and never fed
        back to execution; the original tokens go directly to ``subprocess.run`` without a shell.

    Raises:
        OSError: If the executable cannot be started.
        subprocess.CalledProcessError: If the child returns a nonzero status. The module entry point
        prints the shared error format and returns the child's effective failure status.
    """

    # Build a copy intended only for human-readable logging. shlex.quote makes spaces and shell-like
    # characters unambiguous without changing any token in the actual command sequence.
    lines = [shlex.quote(str(command[0]))]

    for arg in command[1:]:
        quoted_arg = shlex.quote(str(arg))

        # Put each option on a new continuation line. Non-option tokens stay beside the option or
        # executable they follow; negative values begin with one dash and therefore remain attached.
        if str(arg).startswith("-"):
            lines.append(quoted_arg)
        else:
            lines[-1] += f" {quoted_arg}"

    # The displayed backslash-newline layout mirrors a copyable multiline shell command, although no
    # shell parses it during execution.
    formatted_command = " \\\n    ".join(lines)

    # Flush before starting the child so buffered parent output cannot appear after child monitoring.
    print(f"{COLOR_INFO}Executing command:{COLOR_END}\n{formatted_command}", flush=True)
    print()

    # Anchor relative child paths to the verified checkout and turn any nonzero result into a checked
    # exception handled consistently by the command-line entry point.
    subprocess.run(command, cwd=ROOT, check=True)
    print()
# endregion


# banner ----------------------------------------------------------------------------------------------------------------------------------------------------------------

# region banner
def banner(name):
    """Print a presentation-only workflow status banner.

    Purpose:
        Reuse the maintained tcsh artwork for success and stopped workflows without allowing optional
        terminal presentation to replace or conceal the workflow's real result.

    Workflow:
        Build the printer path from the repository root and requested suffix, start it with a clean
        noninteractive tcsh, and allow its output to flow directly to the current terminal or log. If
        tcsh cannot be started, print a minimal plain-text status instead.

    Args:
        name: Trusted internal printer suffix, currently values such as ``success`` or ``stop``. It
            selects ``scripts/printers/print_<name>.csh`` and is not shell-evaluated.

    Returns:
        None. Banner completion or failure never changes the status chosen by the calling workflow.

    Outputs:
        Writes the selected artwork, or its plain-text fallback, to standard output. It creates no
        workflow data and does not alter launcher configuration.

    Failure behavior:
        A printer's nonzero exit is intentionally ignored through ``check=False``. An ``OSError``
        while starting tcsh triggers the fallback. The caller remains responsible for printing any
        diagnostic and returning the actual success or failure status.
    """

    # Resolve an absolute path so banner selection is independent of the directory from which the
    # launcher was called. Passing an argv list prevents the suffix or path from being shell source.
    try:
        # Printers are optional presentation helpers: inherit their output, but do not promote their
        # exit status into a build, generation, or submission failure.
        subprocess.run(['tcsh', '-f', str(ROOT / 'scripts/printers' / f'print_{name}.csh')], check=False)
    except OSError:
        # Keep status transitions visible on systems where tcsh itself is unavailable.
        print(f'CLAS12 samples: {name}', flush=True)
# endregion


# main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

# region main
def main():
    """Run the configured checkout workflow in dependency order.

    Purpose:
        Keep sourced-shell wrappers limited to synchronization and environment setup while this
        function owns the ordered build, validation, and dispatch decisions shared by local and ifarm
        use.

    Workflow:
        1. Separate launcher options from child-workflow overrides and load the effective run profile.
        2. Resolve the build directory and merge profile arguments with explicit child overrides.
        3. When enabled, configure CMake and build both LUND applications.
        4. When enabled, verify that testing was configured and require CTest to succeed.
        5. When enabled, run one create-lund source executable or the ifarm submission coordinator.
        6. Print the success banner only after every requested stage completes.

    Args:
        None. Launcher and forwarded child options are read from ``sys.argv``.

    Returns:
        Zero when all enabled stages succeed, including a valid no-build/no-test/no-run configuration.

    Outputs:
        May create or update the configured CMake build tree, run tests, create LUND output, or submit
        Slurm jobs, according to the selected settings. Commands and stage banners are printed as an
        inspectable execution record.

    Raises:
        OSError, ValueError, TypeError, RuntimeError: For invalid configuration or unavailable files.
        subprocess.CalledProcessError: When a checked build, test, generation, or submission command
        fails. The module entry point converts these exceptions to diagnostics and nonzero statuses.
    """

    # Parse launcher-owned flags and retain all unknown tokens for the chosen child workflow. Users
    # may place one bare `--` at the boundary to make that ownership explicit; it is not forwarded.
    args, forwarded = parser().parse_known_args()
    if forwarded[:1] == ['--']:
        forwarded = forwarded[1:]

    # Resolve and validate the complete launcher configuration before performing any external action.
    config = settings(args)

    # Extract the dispatch axes and normalize the configured build location. Relative build paths are
    # anchored to the checkout so invocation from another directory produces the same tree.
    workflow = config['workflow']
    source = config['source']
    build = Path(config['build_dir'])
    if not build.is_absolute():
        build = ROOT / build
    build = build.resolve()

    # create-lund profiles store separate defaults for uniform and physical inputs; submission uses
    # its own entry. Explicit forwarded options replace matching defaults in the chosen entry only.
    argument_key = source if workflow == 'create-lund' else workflow
    arguments = forwarded_arguments(config['arguments'].get(argument_key, []), forwarded)

    # Compile both sample applications before selecting which workflow to execute.
    if config['build']:
        # Stage banners use visible-width padding independent of ANSI color sequences.
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print(f"{COLOR_START}= Compiling applications                                                                           ={COLOR_END}")
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print()

        # Always re-run CMake configuration so dependency checks see local external-file replacements,
        # including targets.h, that cannot be inferred from a Git commit stamp. Enable both LUND apps;
        # enable the test targets only when this invocation requests them.
        execute(['cmake', '-S', str(ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE='+config['build_type'],
                 '-DBUILD_UNIFORM=ON', '-DBUILD_GENIE=ON', '-DBUILD_TESTING='+('ON' if config['test'] else 'OFF')])

        # Let CMake select the underlying build tool while honoring configured parallelism.
        execute(['cmake', '--build', str(build), '--parallel', str(config['jobs'])])

        print()
    
    # Tests must succeed before the selected workflow can run.
    if config['test']:
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print(f"{COLOR_START}= Running tests                                                                                    ={COLOR_END}")
        print(f"{COLOR_START}===================================================================================================={COLOR_END}")
        print()

        # This guard matters when `--test true` reuses a tree with `--build false`: CTest cannot be
        # assumed available merely because the directory exists. A missing cache propagates as OSError.
        cache = (build / 'CMakeCache.txt').read_text()

        if 'BUILD_TESTING:BOOL=ON' not in cache:
            raise RuntimeError(error_message('Tests are not configured; use --build true --test true'))

        # Show complete diagnostics for any failing test and stop before generation or submission.
        execute(['ctest', '--test-dir', str(build), '--output-on-failure'])

        print()

    # Dispatch exactly one workflow; generation does not automatically launch GEMC.
    if config['run']:
        if workflow == 'create-lund':
            # Uniform samples use the random-kinematics application. Physical inputs use the generic
            # event-generator converter, whose `--event-generator` option currently defaults to GENIE.
            app = 'clas12-uniform' if source == 'uniform' else 'clas12-generator-to-lund'

            # Compute padding from uncolored text because terminal escape sequences occupy no columns.
            message = f"Creating LUND files from '{COLOR_END}{source}{COLOR_START}' input"
            visible_length = len(f"Creating LUND files from '{source}' input")
            padding = 96 - visible_length

            print(f"{COLOR_START}===================================================================================================={COLOR_END}")
            print(f"{COLOR_START}= {message}{' ' * padding} ={COLOR_END}")
            print(f"{COLOR_START}===================================================================================================={COLOR_END}")
            print()

            executable = build / 'apps' / app

            # With building disabled, require the selected application to exist in the requested tree
            # and give the user the direct recovery action rather than failing inside subprocess.run.
            if not executable.is_file():
                raise RuntimeError(error_message(f'Executable missing: {executable}; enable --build true'))

            command = [str(executable)]
        else:
            # Submission is a maintained Python coordinator. It validates inputs and invokes the
            # protected GEMC/reconstruction payload; it does not run detector simulation locally.
            command = [sys.executable, str(ROOT / 'scripts/slurm/submit.py')]

        # Append the already validated child argv without shell parsing or string reconstruction.
        execute(command + arguments)

    # Reaching this point means every stage requested by the configuration completed successfully.
    banner('success')

    return 0
# endregion


# Command-line entry point ----------------------------------------------------------------------------------------------------------------------------------------------

# region Execution
# Purpose:
#     Convert main() and its checked subprocesses into stable process exit statuses for run.csh while
#     keeping this module importable by tests and maintenance tools without launching a workflow.
# Workflow:
#     Direct execution calls main(). Successful completion exits with zero; interruption, child-command
#     failure, and launcher-owned exceptions each print the stop banner and an error diagnostic before
#     returning their defined nonzero status. argparse handles its own usage failures with status 2.
# Outputs:
#     Writes presentation and diagnostics to the inherited terminal streams. The final process status
#     becomes `$status` in run.csh's caller and determines whether later shell work may continue.
if __name__ == '__main__':
    try:
        # Use main's return value as the process status. Imports skip this entire block.
        sys.exit(main())
    except KeyboardInterrupt:
        # Ctrl-C is a user interruption rather than an internal validation failure. Status 130 follows
        # the conventional 128 + SIGINT(2) shell representation.
        banner('stop')
        print_error('Interrupted.')
        sys.exit(130)
    except subprocess.CalledProcessError as error:
        # Checked commands return positive exit codes for ordinary failures and negative signal numbers
        # when Python observes signal termination. Preserve positive codes and translate a signal N to
        # the shell convention 128 + N.
        banner('stop')
        exit_status = 128-error.returncode if error.returncode < 0 else error.returncode

        # shlex.join quotes the argv for an unambiguous diagnostic only; the command was already run
        # directly as an argv sequence by execute().
        print_error(f'Command failed with exit status {exit_status}: {shlex.join(map(str, error.cmd))}')
        sys.exit(exit_status)
    except (OSError, ValueError, TypeError, RuntimeError) as error:
        # Configuration, filesystem, and launcher-state failures share status 1. error_message() and
        # print_error() are idempotent, so explicitly prefixed validation errors appear exactly once.
        banner('stop')
        print_error(error)
        sys.exit(1)
# endregion
