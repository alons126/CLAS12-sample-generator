#!/usr/bin/env python3

#
# Created by Alon Sportes on 19/09/2026.
#

"""Resolve submission settings for completed LUND samples.

Purpose:
    Translate a completed LUND manifest and optional user settings into validated values for
    the ifarm submission coordinator. This module does not submit jobs.

Workflow:
    Merge CLI > config > manifest > defaults, verify that explicit truth metadata agrees with
    the manifest, check selected files and detector inputs, then return one environment dictionary
    per distinct sample to submit.py. run.csh uses --check-arguments to reject malformed syntax
    before refreshing the ifarm checkout.

Inputs:
    One or more RUN/lundfiles directories, optional flat key = value configuration, CLI overrides,
    and completed lund-gen-log.json manifests when available.

Outputs:
    Resolved submission values for preview or execution. This module writes no shell assignments
    and performs no software loading, output cleanup, detector execution, or Slurm calls.

Failure:
    Invalid metadata, missing inputs, conflicting truth values, and unsafe paths raise before
    submission starts.

CLI options:
    --lund-dir DIRECTORY          Select completed RUN/lundfiles; repeat for multiple samples.
    --config FILE                 Read optional key = value submission settings.
    --execute                     Replace simulation outputs and submit; default: preview.
    --source uniform|physical     Set source when no completed manifest supplies it.
    --beam-energy GeV             Set truth beam energy when no manifest supplies it.
    --rgm-target ID               Set truth target identity when no manifest supplies it.
    --channel NAME                Set uniform 1e, eh, electron-tester, or a legacy channel label.
    --hadron NAME                 Set proton, neutron, pip, or pim for an eh channel.
    --hadron-region FD|CD         Select the eh hadron detector region.
    --event-generator NAME        Set physical generator; default: genie without a manifest.
    --tune NAME                   Set physical tune; default: unknown without a manifest.
    --q2-cut NAME                 Record physical input Q2 label; no cut is applied here.
    --prefix NAME                 Set LUND filename prefix; required without a manifest.
    --gemc-version VERSION        Select GEMC resources; fallback default: 5.14.
    --gemc-target-variation NAME  Select detector target variation.
    --gcard FILE / --yaml FILE    Override detector and reconstruction inputs.
    --torus SCALE                 Override the beam-dependent torus default.
    --num-jobs N                  Select the first N LUND files; default: all completed files.
    --events-per-job N            Set event limit; required without a manifest.
    --job-name NAME               Override the metadata-derived Slurm job name.
    --clas12tags-dir DIRECTORY    Use a custom clas12Tags checkout as GEMC_DATA_DIR.
    --clear-farm-out true|false   Clear direct farm log files with --execute; default: false.
    --farm-out DIRECTORY          Set farm_out directory when clearing it.
    --fc-status 0|1               Set legacy physical filename/report label; default: 0.
    --help                        Print submission help before any server synchronization.
"""

import argparse
from decimal import Decimal, InvalidOperation, ROUND_HALF_UP
import json
from pathlib import Path
import re
import sys

# Input contract --------------------------------------------------------------

# region Input contract
# OPTIONS is the public CLI vocabulary and the allowlist for flat config keys. The parser uses
# each description as --help text; resolve() consumes the same names, preventing a config-only
# setting that cannot be expressed on the command line. Config paths are relative to the config
# file; CLI paths are relative to the checkout when run.csh is used.
OPTIONS = {
    'lund-dir': 'Completed RUN/lundfiles directory (repeat on CLI for several samples)',
    'source': 'uniform or physical; normally read from the manifest',
    'beam-energy': 'Truth beam energy in GeV',
    'rgm-target': 'Truth target identity, independently of detector target variation',
    'channel': 'Uniform 1e, eh, electron-tester, or an explicit legacy/regional label',
    'hadron': 'proton, neutron, pip or pim when channel=eh',
    'hadron-region': 'FD or CD when channel=eh',
    'event-generator': 'Physical generator label (default: genie for physical input)',
    'tune': 'Physical tune label (default: unknown for physical, none for uniform)',
    'q2-cut': 'Physical input Q2 label, not a cut applied here',
    'prefix': 'Filename prefix before _INDEX.txt; required without a manifest',
    'gemc-version': 'GEMC resource version (fallback default: 5.14)',
    'gemc-target-variation': 'Detector target variation; normally supplied by the manifest',
    'gcard': 'Explicit detector GCARD; otherwise selected from beam/variation/version',
    'yaml': 'Explicit reconstruction YAML; otherwise selected from beam/version',
    'torus': 'Torus scale; defaults to +0.5 at 2 GeV and -1.0 at 4/6 GeV',
    'num-jobs': 'Submit the first N files (default: all completed files)',
    'events-per-job': 'Shared event limit (default: maximum selected manifest file count)',
    'job-name': 'Slurm job name (default: derived from sample metadata)',
    'clas12tags-dir': 'Custom gemc/clas12Tags checkout used as GEMC_DATA_DIR; intended for detector-development tests',
    'clear-farm-out': 'true/false (default: false); delete files directly in farm-out once',
    'farm-out': 'Explicit farm_out directory, required only when clearing it',
    'fc-status': '0 or 1 legacy physical filename/report label only (default: 0)',
}

# Only these user-supplied values represent paths, so read_config() rebases them to the config
# location before CLI overrides are merged. Other settings remain metadata or numeric text.
PATH_KEYS = {'lund-dir', 'gcard', 'yaml', 'clas12tags-dir', 'farm-out'}

# Tokens become filename/job-name components. Worker paths remain more restrictive because the
# protected GEMC payload retains unquoted detector arguments; path_value() enforces SAFE_PATH.
SAFE_TOKEN = re.compile(r'[A-Za-z0-9_][A-Za-z0-9_.-]*\Z')
SAFE_PATH = re.compile(r'/[A-Za-z0-9_./-]+\Z')

# The beam table binds exact MeV labels to detector-resource directories, default torus scales,
# and reviewed reconstruction YAML names. Unsupported beam labels fail during resolution.
BEAMS = {2070: ('2GeV', '0.5', 'rgm_fall2021-cv.yaml'),
         4029: ('4GeV', '-1.0', 'rgm_fall2021-ai_4Gev.yaml'),
         5986: ('6GeV', '-1.0', 'rgm_fall2021-ai_6Gev.yaml')}

# Uniform hadron choices map to legacy filename labels; LABELS also permits completed runs that
# already carry a resolved FD/CD channel instead of the newer channel=eh input form.
HADRONS = {'proton': 'ep', 'neutron': 'en', 'pip': 'epip', 'pim': 'epim'}
LABELS = {'1e', 'electron-tester', 'ep', 'en'} | {label + region for label in HADRONS.values() for region in ('FD', 'CD')}
# endregion Input contract

# Parsing and validation ------------------------------------------------------

# region Parsing
def parser():
    """Build the submission argument parser.

    Purpose:
        Give run.csh, submit.py, and direct help output one consistent option vocabulary.

    Workflow:
        Register the execution switch, optional config path, every public OPTIONS key, and the
        hidden pre-sync syntax check used by run.csh. Only --lund-dir is repeatable.

    Inputs:
        OPTIONS supplies accepted public setting names and their help descriptions.

    Returns:
        A parser that produces input values; semantic validation occurs later in resolve().
    """

    # The epilog describes submission consequences and precedence; argparse prints it with
    # --help while parse_args() also enforces option spelling and value presence.
    p = argparse.ArgumentParser(description='Resolve LUND inputs and preview or submit one Slurm array per sample.',
        epilog='Precedence: CLI > config > manifest > defaults. Conflicting truth metadata is rejected. '
               'With --execute, submission replaces mchipo/reconhipo while preserving lundfiles. '
               'GEMC defaults to 5.14. Use source run.csh --workflow submit --lund-dir RUN/lundfiles.')

    # Execution is opt-in. A config file may supply common settings, while repeated --lund-dir
    # values select several completed samples in one invocation.
    p.add_argument('--execute', action='store_true', help='Submit jobs and replace simulation outputs; default: preview only')
    p.add_argument('--config', type=Path, help='Optional key = value submission settings')

    for key, help_text in OPTIONS.items():
        p.add_argument('--' + key, action='append' if key == 'lund-dir' else 'store', help=help_text)

    # run.csh calls the hidden switch before its server sync. It checks syntax only; the later
    # coordinator still performs full file and metadata validation.
    p.add_argument('--check-arguments', action='store_true', help=argparse.SUPPRESS)

    return p

def read_config(path):
    """Read an optional flat, non-executable submission config.

    Args:
        path: Config file path, or None when no file was selected.

    Returns:
        Accepted key/value strings. Path values are resolved relative to the config file.

    Failure:
        Missing files, malformed lines, duplicate/unknown keys, and empty values raise before
        any submission environment is constructed.
    """

    result = {}

    # No config is a valid input: the manifest and CLI may provide every needed setting.
    if path is None:
        return result

    # Resolve relative config paths from the config's own directory, independently of the
    # caller's working directory or the location of the selected LUND run.
    path = path.resolve(strict=True)

    for number, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()

        # Blank lines and comments are documentation, not settings.
        if not line or line.startswith('#'):
            continue

        # Split only the first '=' so the diagnostic can identify the exact malformed line.
        if '=' not in line:
            raise ValueError(f'{path}:{number}: expected key = value')

        key, value = (part.strip() for part in line.split('=', 1))

        # Repetition is rejected instead of silently choosing a last value; the key allowlist
        # matches the public CLI and prevents unused or misspelled configuration fields.
        if key not in OPTIONS or key in result or not value:
            raise ValueError(f'{path}:{number}: unknown, repeated or empty setting: {key}')

        # Normalize only path-valued settings here. Numeric and metadata text is validated
        # after config and CLI precedence has been resolved.
        if key in PATH_KEYS:
            value = str((path.parent / value).resolve())

        result[key] = value

    return result

def positive(value, name):
    """Convert a decimal setting to a positive Slurm-compatible integer.

    Args:
        value: Candidate value; booleans, zero, signs, and nondecimal text are rejected.
        name: Setting name included in the diagnostic.

    Returns:
        An integer from 1 through 2147483647.

    Failure:
        Invalid or out-of-range values raise ValueError.
    """

    # The explicit bool check matters because Python booleans are integers. The decimal pattern
    # excludes zero, signs, whitespace, and fractions before int() applies the Slurm-sized bound.
    if isinstance(value, bool) or not re.fullmatch(r'[1-9][0-9]*', str(value)) or int(value) > 2147483647:
        raise ValueError(f'{name} must be an integer from 1 to 2147483647')

    return int(value)

def number(value, name):
    """Read a finite decimal without binary rounding at a beam-energy label boundary.

    Args:
        value: Number or decimal text from a manifest, config, or CLI option.
        name: Setting name included in the diagnostic.

    Returns:
        A finite Decimal used for exact validation and MeV-label conversion.

    Failure:
        Nonnumeric, NaN, and infinite values raise ValueError.
    """

    # Decimal retains exact user text through the GeV-to-MeV label calculation. Convert bad
    # syntax to the same setting-specific ValueError used for non-finite numeric values.
    try:
        result = Decimal(str(value))
    except InvalidOperation as error:
        raise ValueError(f'{name} must be a finite number') from error

    if not result.is_finite():
        raise ValueError(f'{name} must be a finite number')

    return result

def token(value, name):
    """Validate one metadata or filename component.

    Args:
        value: Candidate component before it enters a prefix, label, or job name.
        name: Setting name included in the diagnostic.

    Returns:
        The original value as a string after SAFE_TOKEN accepts it.

    Failure:
        Empty values and shell or path metacharacters raise ValueError.
    """

    # Metadata later enters filenames, job names, and the protected worker environment.
    # A full-string match prevents separators or shell syntax from being appended to a label.
    if not SAFE_TOKEN.fullmatch(str(value)):
        raise ValueError(f'{name} must be a nonempty filename-safe label')

    return str(value)

def path_value(value, name):
    """Canonicalize a path for the protected GEMC worker.

    Args:
        value: User or manifest path to resolve; the target need not exist yet.
        name: Setting name included in the diagnostic.

    Returns:
        An absolute Path whose text satisfies the worker's restrictive path contract.

    Failure:
        Unsupported characters, including spaces, raise ValueError.
    """

    # Canonicalize before checking characters so traversal and symbolic-link expansion cannot
    # hide an unsupported path from the downstream worker's restrictive argument contract.
    path = Path(value).resolve()

    if not SAFE_PATH.fullmatch(str(path)):
        raise ValueError(f'{name}: worker paths may contain only letters, digits, /, _, - and .')

    return path

def channel_label(values):
    """Resolve uniform particle choices to the channel label used by LUND filenames.

    Args:
        values: Merged settings containing channel and, for eh, hadron and hadron-region.

    Returns:
        The corresponding 1e, electron-tester, legacy, or species-plus-region label.

    Failure:
        Unknown channels or incomplete eh selections raise ValueError. Filenames are never
        inspected to infer missing particle identity.
    """

    channel = values.get('channel')

    # The newer eh form names particle and detector region separately; the returned label is
    # the historical filename form, such as enFD or epipCD.
    if channel == 'eh':
        if values.get('hadron') not in HADRONS or values.get('hadron-region') not in ('FD', 'CD'):
            raise ValueError('channel=eh requires --hadron proton|neutron|pip|pim and --hadron-region FD|CD')

        return HADRONS[values['hadron']] + values['hadron-region']

    # Already-resolved legacy/regional labels are accepted when submitting existing files.
    # Their identity is taken from configuration or manifest metadata, never guessed from paths.
    if channel not in LABELS:
        raise ValueError('Specify --channel 1e|eh|electron-tester or an explicit legacy/regional label')

    return channel

def read_manifest(lund_dir):
    """Read and validate the final LUND-generation manifest when present.

    Args:
        lund_dir: Completed RUN/lundfiles directory selected for submission.

    Returns:
        A schema-1 manifest dictionary, or None when manual settings are needed.

    Failure:
        An unfinished .json.tmp file, unsupported schema, malformed config, or missing file list
        raises ValueError rather than treating partial output as a completed run.
    """

    # The writer publishes this final JSON only after completing the LUND run. A remaining
    # temporary sibling indicates interrupted publication rather than manual input.
    path = lund_dir / 'lund-gen-monitoring/lund-gen-log.json'

    if not path.exists():
        if path.with_suffix('.json.tmp').exists():
            raise ValueError(f'LUND creation is incomplete: {path}.tmp')

        return None

    # Validate the outer schema here. resolve() checks ordered file paths, per-file counts,
    # and their agreement with written_events after it knows the selected prefix.
    data = json.loads(path.read_text())

    if not isinstance(data, dict) or type(data.get('schema_version')) is not int or data['schema_version'] != 1:
        raise ValueError(f'Unsupported manifest schema: {path}')

    if data.get('workflow') not in ('uniform', 'physical') or not isinstance(data.get('config'), dict):
        raise ValueError(f'Missing manifest workflow/config: {path}')

    if not all(isinstance(key, str) and isinstance(value, str) for key, value in data['config'].items()):
        raise ValueError(f'Manifest config values must be strings: {path}')

    if not isinstance(data.get('files'), list) or not data['files']:
        raise ValueError(f'Manifest contains no completed files: {path}')

    return data
# endregion Parsing

# Sample resolution -----------------------------------------------------------

# region Resolution
def resolve(lund_directory, explicit, root):
    """Validate one completed run and build its submission environment.

    Purpose:
        Connect LUND output to detector submission without changing the truth recorded when those
        LUND files were created. Detector choices may override generation-time plans.

    Workflow:
        Validate the run location; merge manifest, defaults and explicit settings; resolve beam and
        channel labels; check contiguous LUND files and detector inputs; return worker values.

    Args:
        lund_directory: Selected RUN/lundfiles path. The run is derived from this location, never
            from the manifest's historical config.output value.
        explicit: Config and CLI overrides, already combined with CLI precedence.
        root: Checkout root used to locate detector defaults and reject protected output paths.

    Returns:
        One environment dictionary for the submission coordinator. A subset selects files 1..N;
        JOB_NEVENTS is one shared limit for the selected array tasks.

    Failure:
        Unsafe directories, truth conflicts, missing or inconsistent LUND files, invalid settings,
        and missing detector resources raise ValueError before submission begins.
    """

    # The selected lundfiles directory is the authoritative run location. Canonicalizing it first
    # prevents a manifest copied from another machine from redirecting simulation output.
    lund_dir = path_value(lund_directory, 'lund-dir')

    if lund_dir.name != 'lundfiles' or not lund_dir.is_dir():
        raise ValueError('--lund-dir must name an existing RUN/lundfiles directory')

    # The parent is the simulation output location. Reject broad destinations before any
    # manifest is read, because an executable submission later replaces named child folders.
    run = lund_dir.parent

    if run == Path('/') or run == Path.home().resolve() or run == root or run in root.parents:
        raise ValueError(f'Unsafe simulation output directory: {run}')

    # Protected repository resources can never be used as simulation output locations.
    for protected in (root / 'legacy', root / 'config/detector', root / 'src/slurm-submission/external', root / 'src/lund-generation/external'):
        if run == protected or protected in run.parents:
            raise ValueError(f'Protected output directory: {run}')

    # Inherit only recognized settings from a completed manifest. An unknown or automatic GEMC
    # version is not a usable detector resource selection, so the submission fallback may supply it.
    manifest = read_manifest(lund_dir)
    inherited = {}

    if manifest:
        # Ignore manifest keys outside the submission vocabulary, then preserve the workflow
        # recorded by generation as the source identity for this completed sample.
        inherited = {key: value for key, value in manifest['config'].items() if key in OPTIONS}
        inherited['source'] = manifest['workflow']

        if inherited.get('gemc-version') in ('unknown', 'none', 'auto', ''):
            inherited.pop('gemc-version', None)

        # CLI/config can change simulation policy but must not silently relabel existing truth.
        # Beam energy is compared numerically to allow equivalent decimal spellings; other
        # identity fields must match the manifest text exactly.
        for key in ('source', 'beam-energy', 'rgm-target', 'prefix', 'event-generator', 'tune', 'q2-cut', 'hadron', 'hadron-region'):
            if key in explicit and key in inherited:
                same = number(explicit[key], key) == number(inherited[key], key) if key == 'beam-energy' else explicit[key] == inherited[key]

                if not same:
                    raise ValueError(f'--{key} conflicts with manifest value {inherited[key]!r}')

    # Merge in precedence order and require the minimum identity fields before source-specific
    # checks. Explicit settings have final precedence only where they do not contradict truth.
    values = {'gemc-version': '5.14', 'clear-farm-out': 'false', 'fc-status': '0', **inherited, **explicit}

    for key in ('source', 'beam-energy', 'rgm-target', 'prefix'):
        if not values.get(key):
            raise ValueError(f'Missing --{key}; supply it through the manifest, config or CLI')

    source = values['source']

    if source not in ('uniform', 'physical'):
        raise ValueError('--source must be uniform or physical')

    # The channel determines the uniform file label. Physical events have no uniform channel;
    # supplying uniform-only selectors for physical input is an invocation error.
    channel = channel_label(values) if source == 'uniform' else 'none'

    if manifest and source == 'uniform' and channel != channel_label(inherited):
        raise ValueError('Uniform channel conflicts with manifest particle content')

    if source == 'physical' and any(key in explicit for key in ('channel', 'hadron', 'hadron-region')):
        raise ValueError('Uniform channel settings do not apply to physical input')

    # Decimal arithmetic keeps the three legacy beam labels stable near their nominal energies.
    # Other positive beams require explicit detector files and a torus scale when no table entry exists.
    energy = number(values['beam-energy'], 'beam-energy')

    if energy <= 0:
        raise ValueError('beam-energy must be positive')

    legacy_energies = {Decimal('2.07052'): 2070, Decimal('4.02962'): 4029, Decimal('5.98636'): 5986}
    # Near a production energy, reuse its legacy MeV label. Otherwise round once with
    # ROUND_HALF_UP, avoiding platform-dependent binary floating-point boundary behavior.
    mev = next((label for reference, label in legacy_energies.items() if abs(energy - reference) < Decimal('0.000001')),
               int((energy * 1000).to_integral_value(rounding=ROUND_HALF_UP)))

    # Only the three reviewed beam groups have built-in GCARD/YAML/torus defaults.
    if mev not in BEAMS and not all(values.get(key) for key in ('gcard', 'yaml', 'torus')):
        raise ValueError('No detector defaults for this beam: supply --gcard, --yaml and --torus')

    rounded, default_torus, yaml_name = BEAMS.get(mev, (f'{mev}MeV', values.get('torus'), 'none'))
    torus = number(values.get('torus', default_torus), 'torus')
    torus_text = str(torus)

    # Keep the legacy textual spelling for -1 even though Decimal considers -1 and -1.0 equal.
    if torus == -1:
        torus_text = '-1.0'

    # A manifest states exactly which numbered files and event counts completed. Validate its
    # ordering and total before using those counts to choose a default per-task event limit.
    # Without a manifest, accept only a contiguous numbered sequence for the explicit prefix.
    prefix = token(values['prefix'], 'prefix')
    counts = []

    if manifest:
        # A completed manifest specifies the exact numbered sequence and per-file event counts.
        # Compare the relative path literally so a record cannot redirect a task elsewhere.
        for index, record in enumerate(manifest['files'], 1):
            if not isinstance(record, dict) or record.get('path') != f'lundfiles/{prefix}_{index}.txt':
                raise ValueError('Manifest files must be ordered lundfiles/PREFIX_INDEX.txt starting at 1')

            if type(record.get('events')) is not int:
                raise ValueError('Manifest file event counts must be integers')

            counts.append(positive(record['events'], 'manifest events'))

        # Reject partial or internally inconsistent publication before using any count as an
        # event-limit default for Slurm tasks.
        if type(manifest.get('written_events')) is not int or sum(counts) != manifest['written_events']:
            raise ValueError('Manifest written_events does not equal its file event counts')

        available = len(counts)
    else:
        # Manual input has no published file list. Discover only files with the selected prefix,
        # then require numbering to start at one without gaps or duplicate numeric indices.
        indices = sorted(int(match[1]) for path in lund_dir.iterdir()
                         if (match := re.fullmatch(re.escape(prefix) + r'_([1-9][0-9]*)\.txt', path.name)))

        if not indices or indices != list(range(1, len(indices) + 1)):
            raise ValueError('Expected contiguous PREFIX_1.txt through PREFIX_N.txt; check --prefix and LUND files')

        available = len(indices)

    # The job count may select an initial subset. Check every manifest-listed file when a manifest
    # exists, and every selected file otherwise; reject empty or externally linked inputs.
    jobs = positive(values.get('num-jobs', available), 'num-jobs')

    if jobs > available:
        raise ValueError(f'num-jobs={jobs} exceeds {available} completed files')

    # A manifest is a completed-run claim, so every listed file must still exist. For manual
    # input, only files selected by the requested array size need this content check.
    for index in range(1, (available if manifest else jobs) + 1):
        path = lund_dir / f'{prefix}_{index}.txt'

        if not path.is_file() or path.stat().st_size == 0 or path.resolve().parent != lund_dir:
            raise ValueError(f'Missing, empty or externally linked LUND input: {path}')

    # One JOB_NEVENTS value serves the whole array. A manifest supplies the maximum selected count;
    # manual input must provide it because file lengths cannot be inferred safely from plain text.
    limit = values.get('events-per-job', max(counts[:jobs]) if counts else None)

    if limit is None:
        raise ValueError('Without a manifest, specify --events-per-job explicitly')

    limit = positive(limit, 'events-per-job')

    # Resolve GCARD and YAML from beam group, variation and GEMC version unless explicit paths were
    # supplied. Both concrete files must exist before the coordinator is allowed to submit jobs.
    version = token(values['gemc-version'], 'gemc-version')
    variation = token(values.get('gemc-target-variation', 'none'), 'gemc-target-variation')
    requirements = root / f'config/detector/Generation_files_{rounded}/{version}'

    # An explicit GCARD can cover a nonstandard detector variation. Without one, a concrete
    # variation is needed to form the checked-in card filename.
    if variation == 'none' and 'gcard' not in values:
        raise ValueError('Specify --gemc-target-variation or an explicit --gcard')

    card = path_value(values.get('gcard', requirements / f'{variation}_{rounded}.gcard'), 'gcard')
    yaml = path_value(values.get('yaml', requirements / yaml_name), 'yaml')

    # Existence is checked on the final explicit or derived paths. The detector files themselves
    # remain read-only inputs owned by the GEMC/reconstruction configuration.
    for name, path in (('GCARD', card), ('YAML', yaml)):
        if not path.is_file():
            raise ValueError(f'{name} does not exist: {path}; supply an explicit path if needed')

    # Validate optional operational controls separately from truth and detector metadata.
    # farm_out cleanup requires an explicit existing directory; fc-status only affects legacy
    # report/job-name text and does not apply an event-level fiducial cut.
    for key in ('clear-farm-out',):
        if values[key] not in ('true', 'false'):
            raise ValueError(f'--{key} must be true or false')

    if values['fc-status'] not in ('0', '1'):
        raise ValueError('--fc-status must be 0 or 1 (legacy naming only)')

    optional_paths = {}

    # Keep absent optional paths as empty strings, the representation expected by submit.py.
    # Existing directories are required for any explicitly selected optional path.
    for key in ('clas12tags-dir', 'farm-out'):
        path = path_value(values[key], key) if values.get(key) else None

        if path is not None and not path.is_dir():
            raise ValueError(f'--{key} directory does not exist: {path}')

        optional_paths[key] = str(path) if path else ''

    if values['clear-farm-out'] == 'true' and not optional_paths['farm-out']:
        raise ValueError('--clear-farm-out true requires --farm-out')

    # Form readable labels and a default Slurm name from validated metadata, then hand the
    # coordinator only the environment fields that its simulation payload consumes.
    target = token(values['rgm-target'], 'rgm-target')
    generator = token(values.get('event-generator', 'genie') if source == 'physical' else 'uniform', 'event-generator')
    tune = token(values.get('tune', 'unknown' if source == 'physical' else 'none'), 'tune')
    q2 = token(values.get('q2-cut', 'unknown' if source == 'physical' else 'none'), 'q2-cut')
    beam = f'{mev}MeV'
    fc = '_wFC' if values['fc-status'] == '1' else ''
    default_job = f'Uniform_{channel}_sample_{beam}' if source == 'uniform' else f'{target}_{generator}_{tune}_{beam}_{q2}{fc}_GEMC{version}'
    job = token(values.get('job-name', default_job), 'job-name')

    # These names are the explicit contract consumed by submit.py and exported to the protected
    # Slurm payload. OUTPATH always comes from the validated local run directory, not a manifest
    # path from the machine that originally produced the LUND files.
    return dict(source=source, NUM_OF_JOBS=str(jobs), JOB_NEVENTS=str(limit), BEAM_ENERGY_LABEL=beam,
                DETECTOR_ENERGY_GROUP=rounded, UNIFORM_SAMPLE_CHANNEL=channel, TARGET_VARIATION=variation,
                SAMPLE_TARGET_NUCLEUS=target, SAMPLE_GENERATOR=generator, GENERATOR_TUNE=tune, Q2_CUT=q2,
                OUTPATH=str(run), SAMPLE_FILE_PREFIX=prefix, SLURM_JOB_NAME=job,
                GEMC_VERSION=version, CLEAR_FAR_OUT=values['clear-farm-out'],
                CLAS12TAGS_DIR=optional_paths['clas12tags-dir'], farm_out=optional_paths['farm-out'],
                TORUS_FIELD=torus_text,
                REQUIREMENTS_DIR=str(card.parent), GCARD_FILE=str(card), YAML_FILE=str(yaml),
                FC_STATUS_ENABLED=values['fc-status'], FC_STATUS=fc)
# endregion Resolution

# Invocation resolution -------------------------------------------------------

# region Invocation
def resolve_samples(args, root):
    """Resolve the complete requested submission as one validated batch.

    Purpose:
        Make multi-sample preview or submission all-or-nothing at the input-validation stage.

    Workflow:
        Read the optional config, overlay CLI values, select one or more LUND directories, resolve
        each sample in caller order, reject duplicate output locations, then attach execute policy.

    Args:
        args: Parsed public CLI settings, including repeatable lund-dir and the execute switch.
        root: Checkout root passed to resolve() for detector defaults and path guards.

    Returns:
        Distinct, validated environment dictionaries in CLI order. Each carries
        SUBMISSION_EXECUTE=true or false for the coordinator.

    Failure:
        Missing sample selection, any per-sample failure, or repeated OUTPATH raises before output
        cleanup or submission. Detector setup remains the coordinator's responsibility.
    """

    # The flat config provides invocation-wide values; each CLI option except the repeatable
    # lund-dir replaces its config value before individual manifests are inspected.
    explicit = read_config(args.config)

    for key in OPTIONS:
        value = getattr(args, key.replace('-', '_'))

        if value is not None and key != 'lund-dir':
            explicit[key] = value

    # A CLI lund-dir list takes precedence over a single config lund-dir. Remove that selector
    # from the settings passed to resolve(), where it is represented by the function argument.
    directories = args.lund_dir or ([explicit['lund-dir']] if 'lund-dir' in explicit else [])

    if not directories:
        raise ValueError('provide --lund-dir RUN/lundfiles or a config specifying lund-dir')

    explicit.pop('lund-dir', None)

    # Resolve every sample before returning any to submit.py. Distinct canonical OUTPATH values
    # prevent two selected inputs from targeting and replacing the same simulation directory.
    resolved = [resolve(directory, explicit, root) for directory in directories]

    if len({sample['OUTPATH'] for sample in resolved}) != len(resolved):
        raise ValueError('Each selected sample must have a distinct OUTPATH')

    # Preview is the default; the coordinator uses this flag to decide whether any mutation or
    # sbatch call is allowed after successful validation.
    for sample in resolved:
        sample['SUBMISSION_EXECUTE'] = 'true' if args.execute else 'false'

    return resolved

def main():
    """Perform the launcher's early argument check and return its process status.

    Purpose:
        Reject malformed submission commands before run.csh synchronizes the ifarm checkout.

    Workflow:
        Parse arguments, require an input selector, and accept only the internal
        --check-arguments path. submit.py owns full input resolution and execution.

    Returns:
        Zero for a syntactically valid launcher precheck.

    Failure:
        argparse prints usage and exits nonzero for missing selectors or direct invocation.
    """

    p = parser()
    args = p.parse_args()

    if not args.lund_dir and not args.config:
        p.error('provide --lund-dir RUN/lundfiles or --config FILE')

    if not args.check_arguments:
        p.error('use source run.csh --workflow submit to preview or submit')

    return 0

# The resolver CLI is deliberately limited to the pre-sync syntax check; full submission starts
# from run.csh and reaches resolve_samples() through the maintained coordinator in submit.py.
if __name__ == '__main__':
    sys.exit(main())
# endregion Invocation
