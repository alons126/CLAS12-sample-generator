#!/usr/bin/env python3

#
# Created by Alon Sportes on 19/09/2026.
#

"""Check and prepare settings for completed LUND samples.

Purpose:
    Turn a completed run log and optional user settings into checked values for submission. This file
    does not submit jobs.

Workflow:
    Merge command line, config, run log, and defaults -> reject truth conflicts -> check LUND and
    detector files -> return one settings dictionary per sample.

Inputs:
    One or more RUN/lundfiles directories, optional flat key = value configuration, CLI overrides,
    and completed lund-gen-log.json manifests when available.

Outputs:
    Checked values for preview or execution. This file does not load software, clean output, run
    detectors, or call Slurm.

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
    --event-generator NAME        Set physical input adapter; default: genie-gst without a manifest.
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
# OPTIONS defines both command-line names and allowed config keys. Config paths start at the config
# file; command-line paths start at the checkout when run.csh is used.
OPTIONS = {
    'lund-dir': 'Completed RUN/lundfiles directory (repeat on CLI for several samples)',
    'source': 'uniform or physical; normally read from the manifest',
    'beam-energy': 'Truth beam energy in GeV',
    'rgm-target': 'Truth target identity, independently of detector target variation',
    'channel': 'Uniform 1e, eh, electron-tester, or an explicit legacy/regional label',
    'hadron': 'proton, neutron, pip or pim when channel=eh',
    'hadron-region': 'FD or CD when channel=eh',
    'event-generator': 'Physical input adapter label (default: genie-gst for physical input)',
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
    'clas12tags-dir': 'Custom gemc/clas12Tags checkout used as GEMC_DATA_DIR; intended for detector-development studies',
    'clear-farm-out': 'true/false (default: false); delete files directly in farm-out once',
    'farm-out': 'Explicit farm_out directory, required only when clearing it',
    'fc-status': '0 or 1 legacy physical filename/report label only (default: 0)',
}

# Resolve these path settings from the config file before applying command-line values.
PATH_KEYS = {'lund-dir', 'gcard', 'yaml', 'clas12tags-dir', 'farm-out'}

# Tokens enter filenames and job names. Worker paths use the stricter SAFE_PATH rule.
SAFE_TOKEN = re.compile(r'[A-Za-z0-9_][A-Za-z0-9_.-]*\Z')
SAFE_PATH = re.compile(r'/[A-Za-z0-9_./-]+\Z')

# Map reviewed beam labels to detector directories, torus values, and reconstruction files.
BEAMS = {2070: ('2GeV', '0.5', 'rgm_fall2021-cv.yaml'),
         4029: ('4GeV', '-1.0', 'rgm_fall2021-ai_4Gev.yaml'),
         5986: ('6GeV', '-1.0', 'rgm_fall2021-ai_6Gev.yaml')}

# Map hadron choices to filename labels and accept completed runs that already use FD/CD labels.
HADRONS = {'proton': 'ep', 'neutron': 'en', 'pip': 'epip', 'pim': 'epim'}
LABELS = {'1e', 'electron-tester', 'ep', 'en'} | {label + region for label in HADRONS.values() for region in ('FD', 'CD')}
# endregion Input contract

# Parsing and validation ------------------------------------------------------

# region Parsing
def parser():
    """Build the parser for submission options.

    Purpose:
        Give run.csh and submit.py the same option names.

    Workflow:
        Add execution, config, public settings, and the hidden early syntax check.

    Inputs:
        OPTIONS supplies accepted public setting names and their help descriptions.

    Returns:
        Parser for raw input values. resolve() checks their meaning later.
    """

    # Help explains setting priority and what execution changes.
    p = argparse.ArgumentParser(description='Resolve LUND inputs and preview or submit one Slurm array per sample.',
        epilog='Precedence: CLI > config > manifest > defaults. Conflicting truth metadata is rejected. '
               'With --execute, submission replaces mchipo/reconhipo while preserving lundfiles. '
               'GEMC defaults to 5.14. Use source run.csh --workflow submit --lund-dir RUN/lundfiles.')

    # Preview is the default. Repeat --lund-dir to select several samples.
    p.add_argument('--execute', action='store_true', help='Submit jobs and replace simulation outputs; default: preview only')
    p.add_argument('--config', type=Path, help='Optional key = value submission settings')

    for key, help_text in OPTIONS.items():
        p.add_argument('--' + key, action='append' if key == 'lund-dir' else 'store', help=help_text)

    # run.csh uses this hidden switch for syntax checks before server synchronization.
    p.add_argument('--check-arguments', action='store_true', help=argparse.SUPPRESS)

    return p

def read_config(path):
    """Read an optional plain key-value submission config.

    Args:
        path: Config file path, or None when no file was selected.

    Returns:
        Accepted key/value strings. Path values are resolved relative to the config file.

    Failure:
        Missing files, bad lines, repeated or unknown keys, and empty values raise an error.
    """

    result = {}

    # A run log and command-line options may provide all settings without a config file.
    if path is None:
        return result

    # Resolve relative paths from the config file's directory.
    path = path.resolve(strict=True)

    for number, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()

        # Ignore blank lines and comments.
        if not line or line.startswith('#'):
            continue

        # Split the first `=` and report the exact bad line.
        if '=' not in line:
            raise ValueError(f'{path}:{number}: expected key = value')

        key, value = (part.strip() for part in line.split('=', 1))

        # Reject repeated, unknown, and empty settings.
        if key not in OPTIONS or key in result or not value:
            raise ValueError(f'{path}:{number}: unknown, repeated or empty setting: {key}')

        # Resolve paths now; check numbers and labels after all overrides are applied.
        if key in PATH_KEYS:
            value = str((path.parent / value).resolve())

        result[key] = value

    return result

def positive(value, name):
    """Read a positive integer accepted by Slurm.

    Args:
        value: Candidate value; booleans, zero, signs, and nondecimal text are rejected.
        name: Setting name included in the diagnostic.

    Returns:
        An integer from 1 through 2147483647.

    Failure:
        Invalid or out-of-range values raise ValueError.
    """

    # Reject booleans, zero, signs, spaces, fractions, and values above the Slurm limit.
    if isinstance(value, bool) or not re.fullmatch(r'[1-9][0-9]*', str(value)) or int(value) > 2147483647:
        raise ValueError(f'{name} must be an integer from 1 to 2147483647')

    return int(value)

def number(value, name):
    """Read a finite decimal without floating-point label errors.

    Args:
        value: Number or decimal text from a manifest, config, or CLI option.
        name: Setting name included in the diagnostic.

    Returns:
        A finite Decimal used for checks and the MeV label.

    Failure:
        Nonnumeric, NaN, and infinite values raise ValueError.
    """

    # Decimal keeps the written value exact while the MeV label is calculated.
    try:
        result = Decimal(str(value))
    except InvalidOperation as error:
        raise ValueError(f'{name} must be a finite number') from error

    if not result.is_finite():
        raise ValueError(f'{name} must be a finite number')

    return result

def token(value, name):
    """Check one value used in metadata, filenames, or job names.

    Args:
        value: Candidate component before it enters a prefix, label, or job name.
        name: Setting name included in the diagnostic.

    Returns:
        The original value as a string after SAFE_TOKEN accepts it.

    Failure:
        Empty values and shell or path metacharacters raise ValueError.
    """

    # Require the whole value to match so shell or path characters cannot be added.
    if not SAFE_TOKEN.fullmatch(str(value)):
        raise ValueError(f'{name} must be a nonempty filename-safe label')

    return str(value)

def path_value(value, name):
    """Resolve and check a path for the external GEMC worker.

    Args:
        value: User or manifest path to resolve; the target need not exist yet.
        name: Setting name included in the diagnostic.

    Returns:
        An absolute path containing only characters accepted by the worker.

    Failure:
        Unsupported characters, including spaces, raise ValueError.
    """

    # Resolve `..` and links before checking allowed characters.
    path = Path(value).resolve()

    if not SAFE_PATH.fullmatch(str(path)):
        raise ValueError(f'{name}: worker paths may contain only letters, digits, /, _, - and .')

    return path

def channel_label(values):
    """Build the LUND filename channel from uniform particle choices.

    Args:
        values: Merged settings containing channel and, for eh, hadron and hadron-region.

    Returns:
        The corresponding 1e, electron-tester, legacy, or species-plus-region label.

    Failure:
        Unknown channels or incomplete eh selections raise ValueError. Filenames are never
        inspected to infer missing particle identity.
    """

    channel = values.get('channel')

    # Join the separate hadron and region values into a label such as enFD or epipCD.
    if channel == 'eh':
        if values.get('hadron') not in HADRONS or values.get('hadron-region') not in ('FD', 'CD'):
            raise ValueError('channel=eh requires --hadron proton|neutron|pip|pim and --hadron-region FD|CD')

        return HADRONS[values['hadron']] + values['hadron-region']

    # Existing runs may already contain a complete channel label. Never guess it from a path.
    if channel not in LABELS:
        raise ValueError('Specify --channel 1e|eh|electron-tester or an explicit legacy/regional label')

    return channel

def read_manifest(lund_dir):
    """Read and check the completed LUND run log when present.

    Args:
        lund_dir: Completed RUN/lundfiles directory selected for submission.

    Returns:
        A schema-1 run-log dictionary, or None when manual settings are needed.

    Failure:
        An unfinished file, unsupported schema, invalid config, or missing file list raises ValueError.
    """

    # The final JSON exists only after success. A temporary file means creation was interrupted.
    path = lund_dir / 'lund-gen-monitoring/lund-gen-log.json'

    if not path.exists():
        if path.with_suffix('.json.tmp').exists():
            raise ValueError(f'LUND creation is incomplete: {path}.tmp')

        return None

    # Check the main structure here. resolve() later checks file order and counts.
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
    """Check one completed run and build its submission settings.

    Purpose:
        Prepare detector submission without changing the truth stored with the LUND files.

    Workflow:
        Check the run path -> merge settings -> resolve labels -> check LUND and detector files ->
        return worker values.

    Args:
        lund_directory: Selected RUN/lundfiles path. The run is derived from this location, never
            from the manifest's historical config.output value.
        explicit: Config and CLI overrides, already combined with CLI precedence.
        root: Checkout root used to find detector defaults and reject unsafe output paths.

    Returns:
        One environment dictionary for the submission coordinator. A subset selects files 1..N;
        JOB_NEVENTS is one shared limit for the selected array tasks.

    Failure:
        Unsafe paths, truth conflicts, missing LUND files, invalid settings, or missing detector files
        raise ValueError before submission.
    """

    # Use the selected lundfiles directory as the run location, not a path stored on another machine.
    lund_dir = path_value(lund_directory, 'lund-dir')

    if lund_dir.name != 'lundfiles' or not lund_dir.is_dir():
        raise ValueError('--lund-dir must name an existing RUN/lundfiles directory')

    # The parent holds simulation output. Reject broad paths before reading the run log.
    run = lund_dir.parent

    if run == Path('/') or run == Path.home().resolve() or run == root or run in root.parents:
        raise ValueError(f'Unsafe simulation output directory: {run}')

    # Repository inputs cannot be simulation output directories.
    for protected in (root / 'legacy', root / 'config/detector', root / 'src/slurm-submission/external', root / 'src/lund-generation/external'):
        if run == protected or protected in run.parents:
            raise ValueError(f'Protected output directory: {run}')

    # Read only known settings from the run log. Let submission replace an unknown GEMC version.
    manifest = read_manifest(lund_dir)
    inherited = {}

    if manifest:
        # Ignore unrelated run-log keys and keep its recorded source type.
        inherited = {key: value for key, value in manifest['config'].items() if key in OPTIONS}
        inherited['source'] = manifest['workflow']

        if inherited.get('gemc-version') in ('unknown', 'none', 'auto', ''):
            inherited.pop('gemc-version', None)

        # Overrides may change simulation choices but cannot relabel existing truth.
        for key in ('source', 'beam-energy', 'rgm-target', 'prefix', 'event-generator', 'tune', 'q2-cut', 'hadron', 'hadron-region'):
            if key in explicit and key in inherited:
                same = number(explicit[key], key) == number(inherited[key], key) if key == 'beam-energy' else explicit[key] == inherited[key]

                if not same:
                    raise ValueError(f'--{key} conflicts with manifest value {inherited[key]!r}')

    # Apply setting priority, then require the main sample identity fields.
    values = {'gemc-version': '5.14', 'clear-farm-out': 'false', 'fc-status': '0', **inherited, **explicit}

    for key in ('source', 'beam-energy', 'rgm-target', 'prefix'):
        if not values.get(key):
            raise ValueError(f'Missing --{key}; supply it through the manifest, config or CLI')

    source = values['source']

    if source not in ('uniform', 'physical'):
        raise ValueError('--source must be uniform or physical')

    # Uniform input needs a channel label. Physical input must not receive uniform-only settings.
    channel = channel_label(values) if source == 'uniform' else 'none'

    if manifest and source == 'uniform' and channel != channel_label(inherited):
        raise ValueError('Uniform channel conflicts with manifest particle content')

    if source == 'physical' and any(key in explicit for key in ('channel', 'hadron', 'hadron-region')):
        raise ValueError('Uniform channel settings do not apply to physical input')

    # Decimal arithmetic keeps the three production beam labels exact.
    energy = number(values['beam-energy'], 'beam-energy')

    if energy <= 0:
        raise ValueError('beam-energy must be positive')

    legacy_energies = {Decimal('2.07052'): 2070, Decimal('4.02962'): 4029, Decimal('5.98636'): 5986}
    # Reuse a production label when it matches; otherwise round once to MeV.
    mev = next((label for reference, label in legacy_energies.items() if abs(energy - reference) < Decimal('0.000001')),
               int((energy * 1000).to_integral_value(rounding=ROUND_HALF_UP)))

    # Only the three reviewed beam groups have built-in detector defaults.
    if mev not in BEAMS and not all(values.get(key) for key in ('gcard', 'yaml', 'torus')):
        raise ValueError('No detector defaults for this beam: supply --gcard, --yaml and --torus')

    rounded, default_torus, yaml_name = BEAMS.get(mev, (f'{mev}MeV', values.get('torus'), 'none'))
    torus = number(values.get('torus', default_torus), 'torus')
    torus_text = str(torus)

    # Keep the expected written form `-1.0`.
    if torus == -1:
        torus_text = '-1.0'

    # A run log lists completed files and counts. Manual input must use consecutive numbered files.
    prefix = token(values['prefix'], 'prefix')
    counts = []

    if manifest:
        # Require the exact numbered paths and counts recorded by the completed run.
        for index, record in enumerate(manifest['files'], 1):
            if not isinstance(record, dict) or record.get('path') != f'lundfiles/{prefix}_{index}.txt':
                raise ValueError('Manifest files must be ordered lundfiles/PREFIX_INDEX.txt starting at 1')

            if type(record.get('events')) is not int:
                raise ValueError('Manifest file event counts must be integers')

            counts.append(positive(record['events'], 'manifest events'))

        # Reject a run log whose total does not match its file counts.
        if type(manifest.get('written_events')) is not int or sum(counts) != manifest['written_events']:
            raise ValueError('Manifest written_events does not equal its file event counts')

        available = len(counts)
    else:
        # For manual input, find the selected prefix and require numbering from 1 without gaps.
        indices = sorted(int(match[1]) for path in lund_dir.iterdir()
                         if (match := re.fullmatch(re.escape(prefix) + r'_([1-9][0-9]*)\.txt', path.name)))

        if not indices or indices != list(range(1, len(indices) + 1)):
            raise ValueError('Expected contiguous PREFIX_1.txt through PREFIX_N.txt; check --prefix and LUND files')

        available = len(indices)

    # Select the first requested files and reject missing, empty, or linked inputs.
    jobs = positive(values.get('num-jobs', available), 'num-jobs')

    if jobs > available:
        raise ValueError(f'num-jobs={jobs} exceeds {available} completed files')

    # A completed run must still contain every listed file. Manual input checks selected files only.
    for index in range(1, (available if manifest else jobs) + 1):
        path = lund_dir / f'{prefix}_{index}.txt'

        if not path.is_file() or path.stat().st_size == 0 or path.resolve().parent != lund_dir:
            raise ValueError(f'Missing, empty or externally linked LUND input: {path}')

    # One event limit serves the whole array. Use the largest selected logged count, or require a
    # manual value when no run log exists.
    limit = values.get('events-per-job', max(counts[:jobs]) if counts else None)

    if limit is None:
        raise ValueError('Without a manifest, specify --events-per-job explicitly')

    limit = positive(limit, 'events-per-job')

    # Use explicit detector files or derive them from beam, variation, and GEMC version.
    version = token(values['gemc-version'], 'gemc-version')
    variation = token(values.get('gemc-target-variation', 'none'), 'gemc-target-variation')
    requirements = root / f'config/detector/Generation_files_{rounded}/{version}'

    # Without an explicit GCARD, a detector variation is needed to form its filename.
    if variation == 'none' and 'gcard' not in values:
        raise ValueError('Specify --gemc-target-variation or an explicit --gcard')

    card = path_value(values.get('gcard', requirements / f'{variation}_{rounded}.gcard'), 'gcard')
    yaml = path_value(values.get('yaml', requirements / yaml_name), 'yaml')

    # Check the final detector paths without changing their files.
    for name, path in (('GCARD', card), ('YAML', yaml)):
        if not path.is_file():
            raise ValueError(f'{name} does not exist: {path}; supply an explicit path if needed')

    # Check optional run controls separately. fc-status changes names only; it is not an event cut.
    for key in ('clear-farm-out',):
        if values[key] not in ('true', 'false'):
            raise ValueError(f'--{key} must be true or false')

    if values['fc-status'] not in ('0', '1'):
        raise ValueError('--fc-status must be 0 or 1 (legacy naming only)')

    optional_paths = {}

    # submit.py expects missing optional paths as empty strings. Selected paths must exist.
    for key in ('clas12tags-dir', 'farm-out'):
        path = path_value(values[key], key) if values.get(key) else None

        if path is not None and not path.is_dir():
            raise ValueError(f'--{key} directory does not exist: {path}')

        optional_paths[key] = str(path) if path else ''

    if values['clear-farm-out'] == 'true' and not optional_paths['farm-out']:
        raise ValueError('--clear-farm-out true requires --farm-out')

    # Build readable labels and the default Slurm job name from checked values.
    target = token(values['rgm-target'], 'rgm-target')
    generator = token(values.get('event-generator', 'genie-gst') if source == 'physical' else 'uniform', 'event-generator')
    tune = token(values.get('tune', 'unknown' if source == 'physical' else 'none'), 'tune')
    q2 = token(values.get('q2-cut', 'unknown' if source == 'physical' else 'none'), 'q2-cut')
    beam = f'{mev}MeV'
    fc = '_wFC' if values['fc-status'] == '1' else ''
    default_job = f'Uniform_{channel}_sample_{beam}' if source == 'uniform' else f'{target}_{generator}_{tune}_{beam}_{q2}{fc}_GEMC{version}'
    job = token(values.get('job-name', default_job), 'job-name')

    # Return only values used by submit.py and the external worker. OUTPATH always uses the local run.
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
    """Check all selected samples as one submission batch.

    Purpose:
        Check every sample before any one of them can change output or submit jobs.

    Workflow:
        Read config -> apply command-line values -> resolve samples in order -> reject repeated output
        paths -> add preview or execute mode.

    Args:
        args: Parsed public CLI settings, including repeatable lund-dir and the execute switch.
        root: Checkout root passed to resolve() for detector defaults and path guards.

    Returns:
        Checked sample dictionaries in command-line order, each marked for preview or execution.

    Failure:
        Missing sample selection, any per-sample failure, or repeated OUTPATH raises before output
        cleanup or submission. Detector setup remains the coordinator's responsibility.
    """

    # Command-line values replace config values before individual run logs are checked.
    explicit = read_config(args.config)

    for key in OPTIONS:
        value = getattr(args, key.replace('-', '_'))

        if value is not None and key != 'lund-dir':
            explicit[key] = value

    # A command-line directory list replaces the one config directory.
    directories = args.lund_dir or ([explicit['lund-dir']] if 'lund-dir' in explicit else [])

    if not directories:
        raise ValueError('provide --lund-dir RUN/lundfiles or a config specifying lund-dir')

    explicit.pop('lund-dir', None)

    # Resolve every sample first and reject two samples that use the same output directory.
    resolved = [resolve(directory, explicit, root) for directory in directories]

    if len({sample['OUTPATH'] for sample in resolved}) != len(resolved):
        raise ValueError('Each selected sample must have a distinct OUTPATH')

    # Preview is the default. Only execute mode may change output or call sbatch.
    for sample in resolved:
        sample['SUBMISSION_EXECUTE'] = 'true' if args.execute else 'false'

    return resolved

def main():
    """Check submission argument syntax before server synchronization.

    Purpose:
        Stop malformed commands before run.csh updates the ifarm checkout.

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

# Direct execution performs only the early syntax check. Full submission starts through run.csh.
if __name__ == '__main__':
    sys.exit(main())
# endregion Invocation
