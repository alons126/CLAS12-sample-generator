#
# Created by Alon Sportes on 19/09/2026.
#

#!/usr/bin/env python3

"""Resolve submission inputs into a small, safely quoted C-shell environment.

Purpose:
    bridge completed LUND output to the existing sourced setup without owning module
    loading, output removal, Slurm submission, or detector commands.

Workflow:
    parse CLI/config -> read optional completed manifest -> merge explicit settings ->
    validate truth metadata and selected files -> emit one environment per sample.

Inputs:
    lundfiles directories, optional key=value config, CLI overrides and manifest schema 1.

Outputs:
    whitelisted unset/set/setenv assignments, on stdout or in a private directory supplied
    by setup_and_submit.csh. Every exported name has both tcsh namespaces cleared before it is
    assigned. All samples are validated before any environment file is written.

Failure:
    malformed input, contradictory truth metadata or unsafe values return nonzero. Missing
    metadata is never inferred from parent-directory names. No simulation outputs are modified.

Submission options:
    --execute                     Submit after validation; default behavior is preview only.
    --config FILE                 Read optional key = value submission settings.
    --lund-dir DIRECTORY          Select completed RUN/lundfiles input; repeat for several samples.
    --source uniform|physical     Override or confirm the manifest workflow.
    --beam-energy GeV             Override or confirm truth beam energy.
    --rgm-target ID               Override or confirm truth target identity.
    --channel LABEL               Select uniform 1e, eh, electron-tester, or a resolved channel label.
    --hadron NAME                 Select proton, neutron, pip, or pim when channel is eh.
    --hadron-region FD|CD         Select the uniform hadron detector region.
    --event-generator NAME        Select physical generator metadata (default: genie).
    --tune NAME / --q2-cut NAME  Set physical-input provenance labels.
    --prefix NAME                 Set the LUND filename stem when no manifest supplies it.
    --gemc-version VERSION        Select the GEMC module/version (fallback: 5.14).
    --gemc-target-variation NAME  Select the detector target variation.
    --gcard FILE / --yaml FILE    Override detector-simulation/reconstruction inputs.
    --torus SCALE                 Override the beam-dependent torus scale.
    --num-jobs N                  Submit the first N completed files (default: all).
    --events-per-job N            Set the shared worker event limit (default: largest selected file).
    --job-name NAME               Override the derived Slurm job name.
    --clas12tags-dir DIRECTORY    Use a custom clas12Tags checkout as GEMC_DATA_DIR.
    --clear-farm-out true|false   Remove direct farm_out files once (default: false).
    --farm-out DIRECTORY          Supply the required cleanup directory when clearing farm_out.
    --fc-status 0|1               Set the legacy physical report/filename label only.
    --help                        Print this option list without modifying simulation output.

Precedence and output:
    CLI values override config values, which override manifest values and defaults. With
    --execute, validated samples are passed to setup_and_submit.csh; simulation output directories
    are replaced there while completed LUND files are preserved.
"""

import argparse
from decimal import Decimal, InvalidOperation, ROUND_HALF_UP
import json
from pathlib import Path
import re
import sys

# Input contract --------------------------------------------------------------

# region Input contract
# Public option names are also the only accepted config keys. Paths in a config are relative
# to that config; CLI paths are relative to the checkout when invoked through run.csh.
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
    'gemc-version': 'GEMC module version (fallback default: 5.14)',
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
PATH_KEYS = {'lund-dir', 'gcard', 'yaml', 'clas12tags-dir', 'farm-out'}
# Values entering the sourced file cannot contain shell syntax. Worker paths are more restrictive
# because the protected payload deliberately retains its original unquoted detector arguments.
SAFE_VALUE = re.compile(r'[A-Za-z0-9_./:+ -]*\Z')
SAFE_TOKEN = re.compile(r'[A-Za-z0-9_][A-Za-z0-9_.-]*\Z')
SAFE_PATH = re.compile(r'/[A-Za-z0-9_./-]+\Z')
BEAMS = {2070: ('2GeV', '0.5', 'rgm_fall2021-cv.yaml'),
         4029: ('4GeV', '-1.0', 'rgm_fall2021-ai_4Gev.yaml'),
         5986: ('6GeV', '-1.0', 'rgm_fall2021-ai_6Gev.yaml')}
HADRONS = {'proton': 'ep', 'neutron': 'en', 'pip': 'epip', 'pim': 'epim'}
LABELS = {'1e', 'electron-tester', 'ep', 'en'} | {label + region for label in HADRONS.values() for region in ('FD', 'CD')}
# endregion Input contract

# Parsing and validation ------------------------------------------------------

# region Parsing
def parser():
    """Define the shared CLI/config vocabulary and private shell-integration switches."""
    p = argparse.ArgumentParser(description='Resolve LUND inputs and submit through the sourced shell workflow.',
        epilog='Precedence: CLI > config > manifest > defaults. Conflicting truth metadata is rejected. '
               'With --execute, submission replaces mchipo/reconhipo and uniform rootfiles, preserving lundfiles. '
               'GEMC defaults to 5.14. Use source run.csh --workflow submit --lund-dir RUN/lundfiles.')
    p.add_argument('--execute', action='store_true', help='Submit jobs and replace simulation outputs; default: preview only')
    p.add_argument('--config', type=Path, help='Optional key = value submission settings')
    for key, help_text in OPTIONS.items():
        p.add_argument('--' + key, action='append' if key == 'lund-dir' else 'store', help=help_text)
    p.add_argument('--environment-dir', type=Path, help=argparse.SUPPRESS)
    p.add_argument('--check-arguments', action='store_true', help=argparse.SUPPRESS)
    return p

def read_config(path):
    """Read a flat, non-executable config; reject duplicate/unknown keys and resolve its paths."""
    result = {}
    if path is None:
        return result
    path = path.resolve(strict=True)
    for number, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if '=' not in line:
            raise ValueError(f'{path}:{number}: expected key = value')
        key, value = (part.strip() for part in line.split('=', 1))
        if key not in OPTIONS or key in result or not value:
            raise ValueError(f'{path}:{number}: unknown, repeated or empty setting: {key}')
        if key in PATH_KEYS:
            value = str((path.parent / value).resolve())
        result[key] = value
    return result

def positive(value, name):
    """Require a positive bounded integer usable by the C-shell array loop."""
    if isinstance(value, bool) or not re.fullmatch(r'[1-9][0-9]*', str(value)) or int(value) > 2147483647:
        raise ValueError(f'{name} must be an integer from 1 to 2147483647')
    return int(value)

def number(value, name):
    """Read a finite decimal, avoiding binary rounding at the MeV label boundary."""
    try:
        result = Decimal(str(value))
    except InvalidOperation as error:
        raise ValueError(f'{name} must be a finite number') from error
    if not result.is_finite():
        raise ValueError(f'{name} must be a finite number')
    return result

def token(value, name):
    """Require one plain metadata/filename component, with no shell metacharacters."""
    if not SAFE_TOKEN.fullmatch(str(value)):
        raise ValueError(f'{name} must be a nonempty filename-safe label')
    return str(value)

def path_value(value, name):
    """Resolve a worker path and reject characters unsupported by the protected payload."""
    path = Path(value).resolve()
    if not SAFE_PATH.fullmatch(str(path)):
        raise ValueError(f'{name}: worker paths may contain only letters, digits, /, _, - and .')
    return path

def channel_label(values):
    """Resolve uniform channel content without guessing particle identity from a filename."""
    channel = values.get('channel')
    if channel == 'eh':
        if values.get('hadron') not in HADRONS or values.get('hadron-region') not in ('FD', 'CD'):
            raise ValueError('channel=eh requires --hadron proton|neutron|pip|pim and --hadron-region FD|CD')
        return HADRONS[values['hadron']] + values['hadron-region']
    if channel not in LABELS:
        raise ValueError('Specify --channel 1e|eh|electron-tester or an explicit legacy/regional label')
    return channel

def read_manifest(lund_dir):
    """Read only the final published manifest; an unfinished .tmp is not manual input."""
    path = lund_dir / 'lund-gen-monitoring/lund-gen-log.json'
    if not path.exists():
        if path.with_suffix('.json.tmp').exists():
            raise ValueError(f'LUND creation is incomplete: {path}.tmp')
        return None
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
    """Resolve one run; return only environment values consumed by the existing shell setup.

    Truth fields in an existing manifest must agree with effective explicit input. Detector choices
    may override generation-time plans. File paths are rebased onto the supplied directory, never
    the historical config.output path. A subset selects files 1..N with one common event limit.
    """
    lund_dir = path_value(lund_directory, 'lund-dir')
    if lund_dir.name != 'lundfiles' or not lund_dir.is_dir():
        raise ValueError('--lund-dir must name an existing RUN/lundfiles directory')
    run = lund_dir.parent
    if run == Path('/') or run == Path.home().resolve() or run == root or run in root.parents:
        raise ValueError(f'Unsafe simulation output directory: {run}')
    # Protected repository resources can never be used as simulation output locations.
    for protected in (root / 'legacy', root / 'config/detector', root / 'src/slurm-submission/external', root / 'src/lund-generation/external'):
        if run == protected or protected in run.parents:
            raise ValueError(f'Protected output directory: {run}')
    manifest = read_manifest(lund_dir)
    inherited = {}
    if manifest:
        inherited = {key: value for key, value in manifest['config'].items() if key in OPTIONS}
        inherited['source'] = manifest['workflow']
        if inherited.get('gemc-version') in ('unknown', 'none', 'auto', ''):
            inherited.pop('gemc-version', None)
        # CLI/config can change simulation policy but must not silently relabel existing truth.
        for key in ('source', 'beam-energy', 'rgm-target', 'prefix', 'event-generator', 'tune', 'q2-cut', 'hadron', 'hadron-region'):
            if key in explicit and key in inherited:
                same = number(explicit[key], key) == number(inherited[key], key) if key == 'beam-energy' else explicit[key] == inherited[key]
                if not same:
                    raise ValueError(f'--{key} conflicts with manifest value {inherited[key]!r}')
    values = {'gemc-version': '5.14', 'clear-farm-out': 'false', 'fc-status': '0', **inherited, **explicit}
    for key in ('source', 'beam-energy', 'rgm-target', 'prefix'):
        if not values.get(key):
            raise ValueError(f'Missing --{key}; supply it through the manifest, config or CLI')
    source = values['source']
    if source not in ('uniform', 'physical'):
        raise ValueError('--source must be uniform or physical')
    channel = channel_label(values) if source == 'uniform' else 'none'
    if manifest and source == 'uniform' and channel != channel_label(inherited):
        raise ValueError('Uniform channel conflicts with manifest particle content')
    if source == 'physical' and any(key in explicit for key in ('channel', 'hadron', 'hadron-region')):
        raise ValueError('Uniform channel settings do not apply to physical input')
    energy = number(values['beam-energy'], 'beam-energy')
    if energy <= 0:
        raise ValueError('beam-energy must be positive')
    # Match RunConfig::beamMeV: retain historical labels near the three production energies,
    # otherwise round positive energies to the nearest MeV.
    legacy_energies = {Decimal('2.07052'): 2070, Decimal('4.02962'): 4029, Decimal('5.98636'): 5986}
    mev = next((label for reference, label in legacy_energies.items() if abs(energy - reference) < Decimal('0.000001')),
               int((energy * 1000).to_integral_value(rounding=ROUND_HALF_UP)))
    if mev not in BEAMS and not all(values.get(key) for key in ('gcard', 'yaml', 'torus')):
        raise ValueError('No detector defaults for this beam: supply --gcard, --yaml and --torus')
    rounded, default_torus, yaml_name = BEAMS.get(mev, (f'{mev}MeV', values.get('torus'), 'none'))
    torus = number(values.get('torus', default_torus), 'torus')
    torus_text = str(torus)
    if torus == -1:
        torus_text = '-1.0'  # Preserve the legacy filenames and printed field scale.
    prefix = token(values['prefix'], 'prefix')
    counts = []
    if manifest:
        for index, record in enumerate(manifest['files'], 1):
            if not isinstance(record, dict) or record.get('path') != f'lundfiles/{prefix}_{index}.txt':
                raise ValueError('Manifest files must be ordered lundfiles/PREFIX_INDEX.txt starting at 1')
            if type(record.get('events')) is not int:
                raise ValueError('Manifest file event counts must be integers')
            counts.append(positive(record['events'], 'manifest events'))
        if type(manifest.get('written_events')) is not int or sum(counts) != manifest['written_events']:
            raise ValueError('Manifest written_events does not equal its file event counts')
        available = len(counts)
    else:
        indices = sorted(int(match[1]) for path in lund_dir.iterdir()
                         if (match := re.fullmatch(re.escape(prefix) + r'_([1-9][0-9]*)\.txt', path.name)))
        if not indices or indices != list(range(1, len(indices) + 1)):
            raise ValueError('Expected contiguous PREFIX_1.txt through PREFIX_N.txt; check --prefix and LUND files')
        available = len(indices)
    jobs = positive(values.get('num-jobs', available), 'num-jobs')
    if jobs > available:
        raise ValueError(f'num-jobs={jobs} exceeds {available} completed files')
    for index in range(1, (available if manifest else jobs) + 1):
        path = lund_dir / f'{prefix}_{index}.txt'
        if not path.is_file() or path.stat().st_size == 0 or path.resolve().parent != lund_dir:
            raise ValueError(f'Missing, empty or externally linked LUND input: {path}')
    limit = values.get('events-per-job', max(counts[:jobs]) if counts else None)
    if limit is None:
        raise ValueError('Without a manifest, specify --events-per-job explicitly')
    limit = positive(limit, 'events-per-job')
    version = token(values['gemc-version'], 'gemc-version')
    variation = token(values.get('gemc-target-variation', 'none'), 'gemc-target-variation')
    requirements = root / f'config/detector/Generation_files_{rounded}/{version}'
    if variation == 'none' and 'gcard' not in values:
        raise ValueError('Specify --gemc-target-variation or an explicit --gcard')
    card = path_value(values.get('gcard', requirements / f'{variation}_{rounded}.gcard'), 'gcard')
    yaml = path_value(values.get('yaml', requirements / yaml_name), 'yaml')
    for name, path in (('GCARD', card), ('YAML', yaml)):
        if not path.is_file():
            raise ValueError(f'{name} does not exist: {path}; supply an explicit path if needed')
    for key in ('clear-farm-out',):
        if values[key] not in ('true', 'false'):
            raise ValueError(f'--{key} must be true or false')
    if values['fc-status'] not in ('0', '1'):
        raise ValueError('--fc-status must be 0 or 1 (legacy naming only)')
    optional_paths = {}
    for key in ('clas12tags-dir', 'farm-out'):
        path = path_value(values[key], key) if values.get(key) else None
        if path is not None and not path.is_dir():
            raise ValueError(f'--{key} directory does not exist: {path}')
        optional_paths[key] = str(path) if path else ''
    if values['clear-farm-out'] == 'true' and not optional_paths['farm-out']:
        raise ValueError('--clear-farm-out true requires --farm-out')
    target = token(values['rgm-target'], 'rgm-target')
    generator = token(values.get('event-generator', 'genie') if source == 'physical' else 'uniform', 'event-generator')
    tune = token(values.get('tune', 'unknown' if source == 'physical' else 'none'), 'tune')
    q2 = token(values.get('q2-cut', 'unknown' if source == 'physical' else 'none'), 'q2-cut')
    beam = f'{mev}MeV'
    fc = '_wFC' if values['fc-status'] == '1' else ''
    default_job = f'Uniform_{channel}_sample_{beam}' if source == 'uniform' else f'{target}_{generator}_{tune}_{beam}_{q2}{fc}_GEMC{version}'
    job = token(values.get('job-name', default_job), 'job-name')
    return dict(source=source, NUM_OF_JOBS=str(jobs), JOB_NEVENTS=str(limit), BEAM_ENERGY_LABEL=beam,
                DETECTOR_ENERGY_GROUP=rounded, UNIFORM_SAMPLE_CHANNEL=channel, TARGET_VARIATION=variation,
                SAMPLE_TARGET_NUCLEUS=target, SAMPLE_GENERATOR=generator, GENERATOR_TUNE=tune, Q2_CUT=q2,
                OUTPATH=str(run), SAMPLE_FILE_PREFIX=prefix, SLURM_JOB_NAME=job,
                GEMC_VERSION=version, CLEAR_FAR_OUT=values['clear-farm-out'],
                CLAS12TAGS_DIR=optional_paths['clas12tags-dir'], farm_out=optional_paths['farm-out'],
                TORUS_FIELD=torus_text,
                REQUIREMENTS_PATH=str(card.parent), GCARD_FILE=str(card), YAML_FILE=str(yaml),
                FC_STATUS_ENABLED=values['fc-status'], FC_STATUS=fc)
# endregion Resolution

# Environment handoff ---------------------------------------------------------

# region Handoff
def shell_environment(values):
    """Encode known settings after clearing stale tcsh local and environment values."""
    lines = []
    for key, value in values.items():
        if not SAFE_VALUE.fullmatch(value):
            raise ValueError(f'Unsupported shell characters in resolved setting {key}')
        if key in ('source', 'farm_out'):
            lines.extend((f'unset {key}', f'set {key} = "{value}"'))
        else:
            # tcsh permits a local and exported variable with the same name. The local value wins
            # during `$name` expansion, so clearing only the environment is not sufficient.
            lines.extend((f'unset {key}', f'unsetenv {key}', f'setenv {key} "{value}"'))
    return '\n'.join(lines) + '\n'

def main():
    """Resolve all requested runs, then emit assignments; return before any shell work on error."""
    p = parser()
    args = p.parse_args()
    if not args.lund_dir and not args.config:
        p.error('provide --lund-dir RUN/lundfiles or --config FILE')
    if args.check_arguments:
        return 0
    explicit = read_config(args.config)
    for key in OPTIONS:
        value = getattr(args, key.replace('-', '_'))
        if value is not None and key != 'lund-dir':
            explicit[key] = value
    directories = args.lund_dir or ([explicit['lund-dir']] if 'lund-dir' in explicit else [])
    if not directories:
        p.error('configuration must specify lund-dir, or provide --lund-dir on the CLI')
    explicit.pop('lund-dir', None)
    root = Path(__file__).resolve().parents[2]
    resolved = [resolve(directory, explicit, root) for directory in directories]
    if len({sample['OUTPATH'] for sample in resolved}) != len(resolved):
        raise ValueError('Each selected sample must have a distinct OUTPATH')
    for sample in resolved:
        sample['SUBMISSION_EXECUTE'] = 'true' if args.execute else 'false'
    contents = [shell_environment(sample) for sample in resolved]
    if args.environment_dir:
        for index, content in enumerate(contents, 1):
            with (args.environment_dir / f'{index:06d}.csh').open('x') as output:
                output.write(content)
    else:
        sys.stdout.write('\n'.join(contents))
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, TypeError, KeyError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
# endregion Handoff
