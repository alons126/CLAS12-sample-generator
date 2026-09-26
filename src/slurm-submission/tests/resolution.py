#
# Created by Alon Sportes on 20/09/2026.
#

"""Test LUND input handling without loading modules or submitting jobs.

Workflow:
    Create portable run logs -> apply defaults and overrides -> reject conflicts, bad records, and
    unsafe shell text. Also check real uniform output when its executable is available.

Inputs:
    Repository root and optional built uniform executable. All test files are temporary, and detector
    inputs stay unchanged.

Outputs:
    Assertions report any mismatch.
"""

import copy
import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile

# Resolver contract -----------------------------------------------------------

# region Tests
project = Path(sys.argv[1]).resolve()
helper = project / 'src/slurm-submission/resolve_inputs.py'
module = runpy.run_path(str(helper))
resolve = module['resolve']

with tempfile.TemporaryDirectory(prefix='clas12-resolve-') as directory:
    root = Path(directory).resolve()
    lund = root / 'copied-sample/lundfiles'
    monitoring = lund / 'lund-gen-monitoring'

    monitoring.mkdir(parents=True)

    manifest_path = monitoring / 'lund-gen-log.json'
    prefix = 'Uniform_sample_enFD_2070MeV'

    for index in (1, 2):
        (lund / f'{prefix}_{index}.txt').write_text('fixture\n')

    manifest = {'schema_version': 1, 'workflow': 'uniform', 'written_events': 4,
                'config': {'output': '/old/computer/sample', 'beam-energy': '2.07052', 'rgm-target': 'Ar40',
                           'gemc-target-variation': 'rgm_fall2021_Ar', 'channel': 'eh', 'hadron': 'neutron',
                           'hadron-region': 'FD', 'prefix': prefix, 'gemc-version': 'unknown'},
                'files': [{'path': f'lundfiles/{prefix}_1.txt', 'events': 3},
                          {'path': f'lundfiles/{prefix}_2.txt', 'events': 1}]}

    def save(data):
        """Publish a controlled completed-manifest fixture."""

        manifest_path.write_text(json.dumps(data))

    def rejected(overrides=None, data=None):
        """Require an invalid input to fail before returning resolved settings."""

        save(manifest if data is None else data)

        try:
            resolve(lund, overrides or {}, project)
        except (ValueError, OSError):
            return

        raise AssertionError(f'Unexpected acceptance: {overrides}, {data}')

    save(manifest)

    result = resolve(lund, {}, project)
    assert result['GEMC_VERSION'] == '5.14'
    assert result['OUTPATH'] == str(lund.parent)
    assert 'OUTPATH_BASE' not in result
    assert result['NUM_OF_JOBS'] == '2' and result['JOB_NEVENTS'] == '3'
    assert result['UNIFORM_SAMPLE_CHANNEL'] == 'enFD' and result['BEAM_ENERGY_LABEL'] == '2070MeV'
    assert result['DETECTOR_ENERGY_GROUP'] == '2GeV'
    assert result['TORUS_FIELD'] == '0.5'
    assert result['CLAS12TAGS_DIR'] == result['farm_out'] == ''
    # Use temporary resources for a nondefault version so detector files stay unchanged.
    card, yaml = root / 'custom.gcard', root / 'custom.yaml'

    card.write_text('<gcard/>')
    yaml.write_text('test: true\n')

    explicit = {'gemc-version': '5.15', 'gcard': str(card), 'yaml': str(yaml), 'torus': '-0.75',
                'num-jobs': '1', 'events-per-job': '2'}
    result = resolve(lund, explicit, project)
    assert result['GEMC_VERSION'] == '5.15' and result['TORUS_FIELD'] == '-0.75'
    assert result['NUM_OF_JOBS'] == '1' and result['JOB_NEVENTS'] == '2'
    custom_tags = root / 'custom-clas12Tags'

    custom_tags.mkdir()

    result = resolve(lund, {**explicit, 'clas12tags-dir': str(custom_tags)}, project)
    assert result['CLAS12TAGS_DIR'] == str(custom_tags)
    planned = copy.deepcopy(manifest)
    planned['config']['gemc-version'] = '5.15'

    save(planned)
    assert resolve(lund, {'gcard': str(card), 'yaml': str(yaml)}, project)['GEMC_VERSION'] == '5.15'
    save(manifest)

    config = root / 'submission.conf'

    config.write_text('lund-dir = copied-sample/lundfiles\ngemc-version = 5.15\ngcard = custom.gcard\nyaml = custom.yaml\nnum-jobs = 1\n')

    args = module['parser']().parse_args(['--config', str(config), '--gemc-version', '5.14'])
    result = module['resolve_samples'](args, project)[0]
    assert result['GEMC_VERSION'] == '5.14' and result['NUM_OF_JOBS'] == '1'
    assert result['GCARD_FILE'] == str(card) and result['SUBMISSION_EXECUTE'] == 'false'

    for overrides in ({'beam-energy': '4.02962'}, {'rgm-target': 'C12'}, {'source': 'physical'},
                      {'hadron': 'proton'}, {'channel': '1e'}, {'prefix': 'wrong'}, {'num-jobs': '3'},
                      {'events-per-job': '0'}, {'torus': 'NaN'}, {'job-name': '$(touch bad)'},
                      {'gemc-version': '5.14;touch-bad'}):
        rejected(overrides)

    for mutate in (lambda d: d.update(written_events=5),
                   lambda d: d['files'][0].update(path='../outside.txt'),
                   lambda d: d['files'][0].update(events=True),
                   lambda d: d.update(schema_version=2)):
        broken = copy.deepcopy(manifest)

        mutate(broken)
        rejected(data=broken)

    custom_beam = copy.deepcopy(manifest)
    custom_beam['config']['beam-energy'] = '8.8'

    save(custom_beam)

    result = resolve(lund, {'gcard': str(card), 'yaml': str(yaml), 'torus': '-1'}, project)
    assert result['BEAM_ENERGY_LABEL'] == result['DETECTOR_ENERGY_GROUP'] == '8800MeV'
    save(manifest)
    (lund / f'{prefix}_2.txt').unlink()
    rejected()
    (lund / f'{prefix}_2.txt').write_text('fixture\n')
    manifest_path.unlink()

    manual = {'source': 'uniform', 'beam-energy': '2.07052', 'rgm-target': 'Ar40', 'channel': 'enFD',
              'prefix': prefix, 'gemc-target-variation': 'rgm_fall2021_Ar', 'events-per-job': '25000'}
    assert resolve(lund, manual, project)['NUM_OF_JOBS'] == '2'
    assert resolve(lund, manual, project)['GEMC_VERSION'] == '5.14'
    unfinished = manifest_path.with_suffix('.json.tmp')

    unfinished.write_text('{}')

    try:
        resolve(lund, manual, project)

        raise AssertionError('Incomplete creation accepted')
    except ValueError:
        pass

    unfinished.unlink()

    # A physical run log uses the same resolver without uniform channel settings.
    physical = copy.deepcopy(manifest)
    physical['workflow'] = 'physical'

    for key in ('channel', 'hadron', 'hadron-region'):
        physical['config'].pop(key)

    physical['config'].update({'event-generator': 'genie-gst', 'tune': 'GEM21_11a_00_000', 'q2-cut': 'Q2_0_02'})
    save(physical)

    result = resolve(lund, {}, project)
    assert result['source'] == 'physical' and result['SAMPLE_GENERATOR'] == 'genie-gst'
    assert result['UNIFORM_SAMPLE_CHANNEL'] == 'none' and result['JOB_NEVENTS'] == '3'

    # When available, use the real generator to check its run-log fields and relative paths.
    if len(sys.argv) > 2:
        subprocess.run([str(Path(sys.argv[2]).resolve()), '--config', str(project / 'config/samples/uniform-1e-2070MeV.conf'),
                        '--events', '4', '--events-per-file', '3', '--output', str(root / 'generated')],
                       check=True, capture_output=True, text=True)

        generated = root / 'generated/Uniform_sample_1e_2070MeV/lundfiles'
        result = resolve(generated, {}, project)
        assert result['NUM_OF_JOBS'] == '2' and result['JOB_NEVENTS'] == '3'
        assert result['UNIFORM_SAMPLE_CHANNEL'] == '1e' and result['GEMC_VERSION'] == '5.14'

print('Manifest portability, defaults, precedence, manual input, partial files and invalid-input checks passed.')
# endregion Tests
