#
# Created by Alon Sportes on 14/09/2026.
#

"""Compare legacy setup transcripts and Slurm handoffs using isolated shell fixtures.

Purpose: exercise the pre-sbatch contract for both sources, all three beam energies,
    and maintained channel extensions without submitting jobs or changing protected files.
Workflow: read archived scripts, relocate settings in temporary copies, intercept module
    and sbatch, and compare complete stdout and exported settings to the unified script.
Inputs: repository root and tcsh executable from CTest. Outputs: assertion diagnostics.
Failure: any unexpected transcript, environment, directory or exit-status difference fails.
"""

import difflib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

PROJECT = Path(sys.argv[1]).resolve()
SHELL = sys.argv[2] if len(sys.argv) > 2 else shutil.which('tcsh')
LEGACY = PROJECT / 'legacy/GEMC-samples'
SETUP = PROJECT / 'src/slurm-submission/setup_and_submit.csh'
PAYLOAD = PROJECT / 'src/slurm-submission/external/submit_GEMC_sample.sh'

# Fixture construction --------------------------------------------------------

# region Fixtures
def fixture(root, source, energy, channel='en', fc=0):
    """Create a self-contained reference/new setup pair and inert module/Slurm tools.

    All script replacements happen under the temporary root. Real detector configuration
    and external payload files are only read or copied. Returns paths, explicit settings,
    and the inherited environment for subsequent success/failure tests.
    """
    root.mkdir(parents=True)
    bin_dir = root / 'bin'
    bin_dir.mkdir()
    run = root / 'output/run'
    for directory in (run / 'lundfiles', root / 'requirements', root / 'tags', root / 'gemc-data', root / 'farm_out'):
        directory.mkdir(parents=True)
    for directory in ('mchipo', 'reconhipo', 'rootfiles'):
        (run / directory).mkdir()
        (run / directory / 'old.hipo').write_text('old simulation output')
    rounded, torus, q2 = {'2070MeV': ('2GeV', '0.5', 'Q2_0_02'),
                          '4029MeV': ('4GeV', '-1.0', 'Q2_0_25'),
                          '5986MeV': ('6GeV', '-1.0', 'Q2_0_40')}[energy]
    target = {'2070MeV': 'rgm_fall2021_C_S', '4029MeV': 'rgm_fall2021_C_L', '5986MeV': 'rgm_fall2021_Cx4'}[energy]
    if source == 'uniform':
        target = 'rgm_fall2021_C_S'  # active archived uniform setting
    tune = 'GEM21_11a_00_000'
    suffix = '_wFC' if fc else ''
    prefix = f'Uniform_{channel}_sample_{energy}' if source == 'uniform' else f'C12_{tune}_{q2}_{energy}'
    job = f'Uniform_{channel}_ConstPn_sample_{energy}' if source == 'uniform' else f'C12_{tune}_{energy}_{q2}{suffix}_GEMC5.14'
    card = root / f'requirements/{target}_{rounded}.gcard'
    yaml_name = {'2070MeV': 'rgm_fall2021-cv.yaml', '4029MeV': 'rgm_fall2021-ai_4Gev.yaml', '5986MeV': 'rgm_fall2021-ai_6Gev.yaml'}[energy]
    yaml = root / f'requirements/{yaml_name}'
    card.write_text('<gcard/>')
    yaml.write_text('test: true\n')
    # The final file is shorter; one configured event limit must still yield one array.
    for index, count in ((1, 3), (2, 1)):
        (run / f'lundfiles/{prefix}_{index}.txt').write_text(('1 1 1 0 0 11 2.07 0 1 1\n1 -1 1 11 0 0 0 0 1 1 0 0 0 0\n') * count)
    payload = root / 'src/slurm-submission/external/submit_GEMC_sample.sh'
    payload.parent.mkdir(parents=True)
    shutil.copy2(PAYLOAD, payload)
    env_file = root / 'scripts/set_env.csh'
    env_file.parent.mkdir(parents=True)
    shutil.copy2(LEGACY / 'scripts/set_env.csh', env_file)
    module = root / 'module.csh'
    module.write_text('echo "$argv" >> "$MODULE_LOG"\nif ("$argv[1]" == "load") setenv GEMC_DATA_DIR "$FIXTURE_GEMC_DATA"\n/bin/sh -c "exit $MODULE_STATUS"\n')
    sbatch = bin_dir / 'sbatch'
    sbatch.write_text('#!' + sys.executable + '\nimport json,os,sys\nwith open(os.environ["SBATCH_LOG"],"a") as f: f.write(json.dumps({"argv":sys.argv[1:],"env":dict(os.environ)})+"\\n")\nsys.exit(int(os.environ.get("SBATCH_STATUS","0")))\n')
    sbatch.chmod(0o755)
    for tool in ('gemc', 'recon-util'):
        path = bin_dir / tool
        path.write_text('#!/bin/sh\nexit 0\n')
        path.chmod(0o755)
    basename = 'uniform_setup_and_submit.csh' if source == 'uniform' else 'genie_job_submission_script.csh'
    old = (LEGACY / 'scripts/setup_and_submission_scripts' / basename).read_text()
    old = re.sub(r'(?m)^setenv NUM_OF_JOBS .*$', 'setenv NUM_OF_JOBS 2', old)
    old = re.sub(r'(?m)^(\s*)setenv OUTPATH_BASE .*$', lambda m: m[1] + f'setenv OUTPATH_BASE {root}/output', old)
    old = re.sub(r'(?m)^(\s*)setenv OUTPATH .*$', lambda m: m[1] + f'setenv OUTPATH {run}', old)
    old = re.sub(r'(?m)^(\s*)setenv CLAS12TAGS_DIR .*$', lambda m: m[1] + f'setenv CLAS12TAGS_DIR {root}/tags', old)
    old = re.sub(r'(?m)^(\s*)setenv REQUIREMENTS_PATH .*$', lambda m: m[1] + f'setenv REQUIREMENTS_PATH {root}/requirements', old)
    old = re.sub(r'(?m)^(\s*)setenv SUBMIT_SCRIPT_FILE .*$', lambda m: m[1] + f'setenv SUBMIT_SCRIPT_FILE {payload}', old)
    old = re.sub(r'(?m)^(\s*)foreach BEAM_E \( .* \)$', lambda m: m[1] + f'foreach BEAM_E ( {energy} )', old)
    old = re.sub(r'(?m)^foreach FC_STATUSES \( .* \)$', f'foreach FC_STATUSES ( {fc} )', old)
    old = re.sub(r'(?m)^(\s*)foreach OUTPATH_PARTICLE \( .* \)$', lambda m: m[1] + f'foreach OUTPATH_PARTICLE ( {channel} )', old)
    old_path = root / 'old.csh'
    old_path.write_text(old)
    values = dict(NUM_OF_JOBS='2', JOB_NEVENTS='3', TEMP_BEAM_E=energy,
                  TEMP_OUTPATH_PARTICLE=channel if source == 'uniform' else 'none', TARGET_VARIATION=target,
                  SAMPLE_TARGET_NUCLEUS='C12', SAMPLE_GENERATOR='uniform' if source == 'uniform' else 'genie',
                  GENERATOR_TUNE=tune, Q2_CUT=q2, OUTPATH_BASE=str(root / 'output'), OUTPATH=str(run),
                  SAMPLE_FILE_PREFIX=prefix, SLURM_JOB_NAME=job, TEMP_BEAM_E_ROUNDED=rounded,
                  TORUS_FIELD=torus, REQUIREMENTS_PATH=str(root / 'requirements'), GCARD_FILE=str(card),
                  YAML_FILE=str(yaml), FC_STATUS_ENABLED=str(fc), FC_STATUS=suffix)
    settings = {
        'lund-dir': str(run / 'lundfiles'), 'source': source, 'beam-energy': str(int(energy[:-3]) / 1000),
        'channel': channel, 'rgm-target': 'C12', 'gemc-target-variation': target, 'prefix': prefix,
        'num-jobs': '2', 'events-per-job': '3', 'tune': tune, 'q2-cut': q2, 'job-name': job,
        'gcard': str(card), 'yaml': str(yaml), 'clas12tags-dir': str(root / 'tags'),
        'farm-out': str(root / 'farm_out'), 'fc-status': str(fc),
    }
    if source == 'physical':
        settings.pop('channel')
    config = root / 'submission.conf'
    config.write_text('\n'.join(f'{key} = {value}' for key, value in settings.items()) + '\n')
    shutil.copy2(PROJECT / 'src/slurm-submission/resolve_inputs.py', payload.parent.parent / 'resolve_inputs.py')
    new_path = root / 'new.csh'
    # Test wrapper supplies a normal config, leaving the maintained setup completely unmodified.
    shutil.copy2(SETUP, payload.parent.parent / 'setup_and_submit.csh')
    new_path.write_text(f'source src/slurm-submission/setup_and_submit.csh --config "{config}" $argv:q\n')
    env = dict(os.environ, PATH=str(bin_dir) + os.pathsep + os.environ['PATH'], MODULE_LOG=str(root / 'modules.log'),
               SBATCH_LOG=str(root / 'sbatch.jsonl'), FIXTURE_GEMC_DATA=str(root / 'gemc-data'),
               MODULE_STATUS='0', GEMC_DATA_DIR='/wrong/inherited/value', SBATCH_STATUS='0')
    return new_path, old_path, values, env

def run_script(path, env, success=True, arguments=(), execute=True):
    """Source a fixture with a shell module alias; verify status and caller-shell survival."""
    if execute:
        arguments = (*arguments, '--execute')
    for key in ('MODULE_LOG', 'SBATCH_LOG'):
        Path(env[key]).unlink(missing_ok=True)
    program = 'source "' + str(PROJECT / 'src/launcher/environment/set_colors.csh') + '"; alias module \'source "' + str(path.parent / 'module.csh') + '" \\!*\'; set script_path="$argv[1]"; shift argv; source "$script_path" $argv:q; set result=$status; echo "SHELL_ALIVE=$result"; /bin/sh -c "exit $result"'
    result = subprocess.run([SHELL, '-f', '-c', program, str(path), *arguments], cwd=path.parent, env=env, capture_output=True, text=True)
    assert f'SHELL_ALIVE={result.returncode}\n' in result.stdout, result.stdout + result.stderr
    assert (result.returncode == 0) == success, result.stdout + result.stderr
    if success:
        assert not result.stderr, result.stdout + result.stderr
    transcript = result.stdout.rsplit('SHELL_ALIVE=', 1)[0]
    calls = [json.loads(line) for line in Path(env['SBATCH_LOG']).read_text().splitlines()] if Path(env['SBATCH_LOG']).exists() else []
    return transcript, calls
# endregion Fixtures

# Parity and failure cases -----------------------------------------------------

# region Tests
with tempfile.TemporaryDirectory(prefix='clas12-setup-parity-') as temp:
    root = Path(temp).resolve()
    for source in ('uniform', 'physical'):
        for energy in ('2070MeV', '4029MeV', '5986MeV'):
            for selection in (('1e', 0), ('ep', 0), ('en', 0)) if source == 'uniform' else (('en', 0), ('en', 1)):
                channel, fc = selection
                new, old, values, env = fixture(root / f'{source}-{energy}-{channel}-{fc}', source, energy, channel, fc)
                reference, old_calls = run_script(old, env)
                # Restore stale outputs so the new setup must perform its own replacement.
                for directory in ('mchipo', 'reconhipo', 'rootfiles'):
                    (Path(values['OUTPATH']) / directory / 'old.hipo').write_text('old simulation output')
                actual, calls = run_script(new, env)
                # The shared palette replaces archived channel colors; compare the complete visible report.
                reference = re.sub(r'(?:\x1b|\\033)\[[0-9;]*m', '', reference)
                actual = re.sub(r'(?:\x1b|\\033)\[[0-9;]*m', '', actual)
                assert reference == actual, ''.join(difflib.unified_diff(reference.splitlines(True), actual.splitlines(True), fromfile='legacy', tofile='unified'))
                assert len(calls) == len(old_calls) == 1
                assert calls[0]['argv'] == old_calls[0]['argv']
                assert calls[0]['argv'][1] == '--array=1-2'
                for name in ('OUTPATH', 'GCARD_FILE', 'YAML_FILE', 'TORUS_FIELD', 'TEMP_BEAM_E', 'GEMC_DATA_DIR', 'ARRAY', 'SLURM_JOB_NAME'):
                    assert calls[0]['env'][name] == old_calls[0]['env'][name], name
                for name, value in values.items():
                    assert calls[0]['env'][name] == value, name
                assert calls[0]['env']['SBATCH_EXPORT'] == calls[0]['env']['SLURM_EXPORT_ENV'] == 'ALL'
                assert Path(env['MODULE_LOG']).read_text() == 'unload gemc\nload gemc/5.14\n'
                assert calls[0]['env']['GEMC_DATA_DIR'] == env['FIXTURE_GEMC_DATA']
                run = Path(values['OUTPATH'])
                assert not list((run / 'mchipo').iterdir()) and not list((run / 'reconhipo').iterdir())
                assert len(list((run / 'lundfiles').glob('*.txt'))) == 2
                assert (run / 'rootfiles/old.hipo').exists() == (source == 'physical')
    for channel in ('enFD', 'enCD', 'epFD', 'epCD', 'epipFD', 'epipCD', 'epimFD', 'epimCD', 'electron-tester'):
        new, _, values, env = fixture(root / channel, 'uniform', '2070MeV', channel)
        _, calls = run_script(new, env)
        assert len(calls) == 1 and calls[0]['env']['TEMP_OUTPATH_PARTICLE'] == channel
    for failure in ('module', 'gcard', 'yaml', 'lund', 'tags', 'symlink', 'unsafe', 'sbatch'):
        new, _, values, env = fixture(root / failure, 'uniform', '2070MeV')
        run = Path(values['OUTPATH'])
        if failure == 'module':
            env['MODULE_STATUS'] = '1'
        elif failure in ('gcard', 'yaml'):
            Path(values['GCARD_FILE' if failure == 'gcard' else 'YAML_FILE']).unlink()
        elif failure == 'lund':
            (run / f'lundfiles/{values["SAMPLE_FILE_PREFIX"]}_2.txt').unlink()
        elif failure == 'tags':
            (new.parent / 'tags').rmdir()
        elif failure == 'symlink':
            shutil.rmtree(run / 'mchipo')
            (run / 'mchipo').symlink_to(new.parent / 'tags', target_is_directory=True)
        elif failure == 'unsafe':
            config = new.parent / 'submission.conf'
            config.write_text(config.read_text().replace(str(run / 'lundfiles'), '/lundfiles'))
        elif failure == 'sbatch':
            env['SBATCH_STATUS'] = '1'
        _, calls = run_script(new, env, success=False)
        assert len(calls) == (1 if failure == 'sbatch' else 0)
        if failure != 'sbatch':
            assert (run / 'reconhipo/old.hipo').exists()
    # Feed a completed manifest through the real resolver and sourced shell, with no sample config.
    for source in ('uniform', 'physical'):
        new, _, values, env = fixture(root / f'manifest-{source}', source, '2070MeV', 'enFD')
        lund = Path(values['OUTPATH']) / 'lundfiles'
        monitoring = lund / 'lund-gen-monitoring'
        monitoring.mkdir()
        metadata = {'beam-energy': '2.07052', 'rgm-target': 'C12', 'prefix': values['SAMPLE_FILE_PREFIX'],
                    'gemc-target-variation': values['TARGET_VARIATION'], 'gemc-version': 'unknown',
                    'output': '/old-machine/run', 'tune': values['GENERATOR_TUNE'], 'q2-cut': values['Q2_CUT']}
        if source == 'uniform':
            metadata.update(channel='eh', hadron='neutron', **{'hadron-region': 'FD'})
        else:
            metadata['event-generator'] = 'genie'
        manifest = {'schema_version': 1, 'workflow': source, 'config': metadata, 'written_events': 4,
                    'files': [{'path': f'lundfiles/{values["SAMPLE_FILE_PREFIX"]}_{index}.txt', 'events': count}
                              for index, count in ((1, 3), (2, 1))]}
        (monitoring / 'lund-gen-log.json').write_text(json.dumps(manifest))
        resources = new.parent / 'config/detector/Generation_files_2GeV/5.14'
        resources.mkdir(parents=True)
        shutil.copy2(values['GCARD_FILE'], resources / Path(values['GCARD_FILE']).name)
        shutil.copy2(values['YAML_FILE'], resources / Path(values['YAML_FILE']).name)
        new.write_text('source src/slurm-submission/setup_and_submit.csh $argv:q\n')
        _, calls = run_script(new, env, arguments=('--lund-dir', str(lund)))
        assert len(calls) == 1 and calls[0]['argv'][1] == '--array=1-2'
        assert calls[0]['env']['GEMC_VERSION'] == '5.14' and calls[0]['env']['JOB_NEVENTS'] == '3'
        assert calls[0]['env']['OUTPATH'] == values['OUTPATH']
        assert calls[0]['env']['TEMP_OUTPATH_PARTICLE'] == ('enFD' if source == 'uniform' else 'none')
        _, calls = run_script(new, env, success=False, arguments=('--lund-dir', str(lund), '--beam-energy', '4.02962'))
        assert not calls

    # Preview must leave both existing and absent output directories unchanged, and never call sbatch.
    new, _, values, env = fixture(root / 'preview', 'uniform', '2070MeV')
    run = Path(values['OUTPATH'])
    log = new.parent / 'farm_out/keep.log'
    log.write_text('preserve log')
    before = {str(path.relative_to(run)): path.read_bytes() for path in run.rglob('*') if path.is_file()}
    output, calls = run_script(new, env, arguments=('--clear-farm-out', 'true'), execute=False)
    assert not calls and 'Preview command (not submitted)' in output and '--array=' in output
    assert log.read_text() == 'preserve log'
    assert before == {str(path.relative_to(run)): path.read_bytes() for path in run.rglob('*') if path.is_file()}
    for folder in ('mchipo', 'reconhipo', 'rootfiles'):
        shutil.rmtree(run / folder)
    (new.parent / 'bin/sbatch').unlink()
    _, calls = run_script(new, env, execute=False)
    assert not calls and all(not (run / folder).exists() for folder in ('mchipo', 'reconhipo', 'rootfiles'))

    # Exercise the real run.csh branch without sync or the Python/build machinery.
    new, _, values, env = fixture(root / 'launcher', 'uniform', '2070MeV')
    checkout = new.parent
    (checkout / '.git').mkdir()
    (checkout / 'src/launcher').mkdir()
    (checkout / 'src/launcher/workflow.py').write_text('raise AssertionError("Submission must bypass Python")\n')
    entry = checkout / 'run.csh'
    shutil.copy2(PROJECT / 'run.csh', entry)
    env['CLAS12_SKIP_SERVER_SYNC'] = '1'
    _, calls = run_script(entry, env, arguments=('--workflow', 'submit', '--config', str(checkout / 'submission.conf')))
    assert len(calls) == 1 and calls[0]['argv'][1] == '--array=1-2'
    _, calls = run_script(entry, env, success=False, arguments=('--workflow', 'submit', '--site', 'removed.json'))
    assert not calls
    env['MODULE_STATUS'] = '1'
    _, calls = run_script(entry, env, success=False, arguments=('--workflow', 'submit', '--config', str(checkout / 'submission.conf')))
    assert not calls

    # Multiple samples repeat setup with distinct outputs and make exactly one array each.
    new, _, values, env = fixture(root / 'multiple', 'uniform', '2070MeV')
    first_run = Path(values['OUTPATH'])
    second_run = first_run.with_name('second')
    shutil.copytree(first_run, second_run)
    _, calls = run_script(new, env, arguments=('--lund-dir', str(first_run / 'lundfiles'), '--lund-dir', str(second_run / 'lundfiles')))
    assert len(calls) == 2
    assert [call['env']['OUTPATH'] for call in calls] == [str(first_run), str(second_run)]
    _, calls = run_script(new, env, success=False, arguments=('--lund-dir', str(first_run / 'lundfiles'), '--lund-dir', str(first_run / 'lundfiles')))
    assert not calls  # Resolve every sample before performing any setup or submission.

    # Preserve the detector command contract, independently of setup presentation.
    command_log = root / 'detector-commands.jsonl'
    stub = '#!' + sys.executable + '\nimport json,os,sys\nwith open(os.environ["COMMAND_LOG"],"a") as f: f.write(json.dumps([os.path.basename(sys.argv[0]),*sys.argv[1:]])+"\\n")\n'
    for tool in ('gemc', 'recon-util'):
        executable = new.parent / 'bin' / tool
        executable.write_text(stub)
        executable.chmod(0o755)
    env['COMMAND_LOG'] = str(command_log)
    for source in ('uniform', 'GENIE'):
        for energy, torus in (('2070MeV', '0.5'), ('4029MeV', '-1.0'), ('5986MeV', '-1.0')):
            prefix = f'Uniform_en_sample_{energy}' if source == 'uniform' else f'C12_GEM21_11a_00_000_Q2_0_02_{energy}'
            worker_env = dict(env, OUTPATH=str(first_run), TEMP_BEAM_E=energy, TEMP_OUTPATH_PARTICLE='en',
                              SAMPLE_TARGET_NUCLEUS='C12', GENIE_TUNE='GEM21_11a_00_000', GENERATOR_TUNE='GEM21_11a_00_000',
                              SAMPLE_GENERATOR=source, Q2_CUT='Q2_0_02', TORUS_FIELD=torus,
                              GCARD_FILE=values['GCARD_FILE'], YAML_FILE=values['YAML_FILE'],
                              SLURM_ARRAY_TASK_ID='1', SAMPLE_FILE_PREFIX=prefix, JOB_NEVENTS='10000')
            logs = []
            for payload in (LEGACY / f'scripts/job_submission_scripts/submit_GEMC_{source}_sample.sh', PAYLOAD):
                command_log.unlink(missing_ok=True)
                subprocess.run(['bash', str(payload)], env=worker_env, check=True, capture_output=True, text=True)
                logs.append(command_log.read_text())
            assert logs[0] == logs[1]
    legacy_payload = (LEGACY / 'scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh').read_bytes()
    assert PAYLOAD.read_bytes().split(b'JOB_TARGET=')[0] == legacy_payload.split(b'JOB_TARGET=')[0]
print('Full legacy setup stdout, arrays/environment, launcher, detector argv and failure tests passed.')
# endregion Tests
