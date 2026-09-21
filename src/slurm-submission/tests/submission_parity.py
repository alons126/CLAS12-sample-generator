#
# Created by Alon Sportes on 14/09/2026.
#

"""Verify the Python submission contract without real Slurm or detector execution.

Workflow:
    isolated checkout fixtures -> sourced bridge -> inert sbatch capture. Golden
    reports were captured from the working C-shell coordinator before its Python migration.
    Cover both source types, beam/channel variations, preview, failures and array environments.
    Protected payload execution uses only fake gemc/recon-util commands in temporary directories.
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
SETUP = PROJECT / 'src/slurm-submission/setup_and_submit.csh'
PAYLOAD = PROJECT / 'src/slurm-submission/external/submit_GEMC_sample.sh'

# Fixture construction --------------------------------------------------------

# region Fixtures
def fixture(root, source, energy, channel='en', fc=0):
    """Create a self-contained checkout with inert Slurm and detector tools.

    All script replacements happen under the temporary root. Real detector configuration
    and external payload files are only read or copied. Returns paths, explicit settings,
    and the inherited environment for subsequent success/failure tests.
    """
    root.mkdir(parents=True)
    bin_dir = root / 'bin'
    bin_dir.mkdir()
    (bin_dir / 'python3').symlink_to(sys.executable)
    run = root / 'output/run'
    for directory in (run / 'lundfiles', root / 'requirements', root / 'tags', root / 'gemc-data', root / 'farm_out'):
        directory.mkdir(parents=True)
    for directory in ('mchipo', 'reconhipo'):
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
    sbatch = bin_dir / 'sbatch'
    sbatch.write_text('#!' + sys.executable + '\nimport json,os,sys\nwith open(os.environ["SBATCH_LOG"],"a") as f: f.write(json.dumps({"argv":sys.argv[1:],"env":dict(os.environ)})+"\\n")\nsys.exit(int(os.environ.get("SBATCH_STATUS","0")))\n')
    sbatch.chmod(0o755)
    for tool in ('gemc', 'recon-util'):
        path = bin_dir / tool
        path.write_text('#!/bin/sh\nexit 0\n')
        path.chmod(0o755)
    values = dict(NUM_OF_JOBS='2', JOB_NEVENTS='3', BEAM_ENERGY_LABEL=energy,
                  UNIFORM_SAMPLE_CHANNEL=channel if source == 'uniform' else 'none', TARGET_VARIATION=target,
                  SAMPLE_TARGET_NUCLEUS='C12', SAMPLE_GENERATOR='uniform' if source == 'uniform' else 'genie',
                  GENERATOR_TUNE=tune, Q2_CUT=q2, OUTPATH=str(run),
                  SAMPLE_FILE_PREFIX=prefix, SLURM_JOB_NAME=job, DETECTOR_ENERGY_GROUP=rounded,
                  TORUS_FIELD=torus, REQUIREMENTS_DIR=str(root / 'requirements'), GCARD_FILE=str(card),
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
    for name in ('resolve_inputs.py', 'submit.py'):
        shutil.copy2(PROJECT / 'src/slurm-submission' / name, payload.parent.parent / name)
    shutil.copytree(PROJECT / 'src/launcher/environment', root / 'src/launcher/environment')
    (run / 'lundfiles/lund-gen-monitoring').mkdir()
    new_path = root / 'new.csh'
    # Test wrapper supplies a normal config, leaving the maintained setup completely unmodified.
    shutil.copy2(SETUP, payload.parent.parent / 'setup_and_submit.csh')
    new_path.write_text(f'source src/slurm-submission/setup_and_submit.csh --config "{config}" $argv:q\n')
    env = dict(os.environ, PATH=str(bin_dir) + os.pathsep + '/usr/bin:/bin',
               SBATCH_LOG=str(root / 'sbatch.jsonl'), GEMC_DATA_DIR=str(root / 'gemc-data'), SBATCH_STATUS='0')
    return new_path, values, env

def run_script(path, env, success=True, arguments=(), execute=True):
    """Source a fixture; verify status and caller-shell survival, including stale locals."""
    if execute:
        arguments = (*arguments, '--execute')
    Path(env['SBATCH_LOG']).unlink(missing_ok=True)
    program = ('set echo_style=both; set OUTPATH=stale; set GEMC_VERSION=stale; '
               'setenv GEMC_VERSION stale; set script_path="$argv[1]"; shift argv; '
               'source "$script_path" $argv:q; set result=$status; '
               'echo "SHELL_ALIVE=$result"; /bin/sh -c "exit $result"')
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
                new, values, env = fixture(root / f'{source}-{energy}-{channel}-{fc}', source, energy, channel, fc)
                actual, calls = run_script(new, env)
                assert len(calls) == 1
                assert calls[0]['argv'] == ['--job-name=' + values['SLURM_JOB_NAME'], '--array=1-2',
                                            str(new.parent / 'src/slurm-submission/external/submit_GEMC_sample.sh')]
                for name, value in values.items():
                    assert calls[0]['env'][name] == value, name
                assert calls[0]['env']['SBATCH_EXPORT'] == calls[0]['env']['SLURM_EXPORT_ENV'] == 'ALL'
                assert calls[0]['env']['GEMC_DATA_DIR'] == str(new.parent / 'tags')
                run = Path(values['OUTPATH'])
                assert not list((run / 'mchipo').iterdir()) and not list((run / 'reconhipo').iterdir())
                assert len(list((run / 'lundfiles').glob('*.txt'))) == 2
                assert not (run / 'rootfiles').exists()
    for channel in ('enFD', 'enCD', 'epFD', 'epCD', 'epipFD', 'epipCD', 'epimFD', 'epimCD', 'electron-tester'):
        new, values, env = fixture(root / channel, 'uniform', '2070MeV', channel)
        _, calls = run_script(new, env)
        assert len(calls) == 1 and calls[0]['env']['UNIFORM_SAMPLE_CHANNEL'] == channel
    for failure in ('gcard', 'yaml', 'lund', 'tags', 'symlink', 'unsafe', 'sbatch'):
        new, values, env = fixture(root / failure, 'uniform', '2070MeV')
        run = Path(values['OUTPATH'])
        if failure in ('gcard', 'yaml'):
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
        new, values, env = fixture(root / f'manifest-{source}', source, '2070MeV', 'enFD')
        lund = Path(values['OUTPATH']) / 'lundfiles'
        monitoring = lund / 'lund-gen-monitoring'
        monitoring.mkdir(exist_ok=True)
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
        assert calls[0]['env']['UNIFORM_SAMPLE_CHANNEL'] == ('enFD' if source == 'uniform' else 'none')
        _, calls = run_script(new, env, success=False, arguments=('--lund-dir', str(lund), '--beam-energy', '4.02962'))
        assert not calls

    # Preview must leave both existing and absent output directories unchanged, and never call sbatch.
    new, values, env = fixture(root / 'preview', 'uniform', '2070MeV')
    run = Path(values['OUTPATH'])
    log = new.parent / 'farm_out/keep.log'
    log.write_text('preserve log')
    before = {str(path.relative_to(run)): path.read_bytes() for path in run.rglob('*') if path.is_file()}
    output, calls = run_script(new, env, arguments=('--clear-farm-out', 'true'), execute=False)
    assert not calls and 'Preview command (not submitted)' in output and '--array=' in output
    assert log.read_text() == 'preserve log'
    assert before == {str(path.relative_to(run)): path.read_bytes() for path in run.rglob('*') if path.is_file()}
    for folder in ('mchipo', 'reconhipo'):
        shutil.rmtree(run / folder)
    (new.parent / 'bin/sbatch').unlink()
    _, calls = run_script(new, env, execute=False)
    assert not calls and all(not (run / folder).exists() for folder in ('mchipo', 'reconhipo'))
    assert not (run / 'rootfiles').exists()

    # Exercise the real run.csh branch without sync or the Python/build machinery.
    new, values, env = fixture(root / 'launcher', 'uniform', '2070MeV')
    checkout = new.parent
    (checkout / '.git').mkdir()
    (checkout / 'src/launcher/workflow.py').write_text('raise AssertionError("Submission must bypass the LUND build driver")\n')
    entry = checkout / 'run.csh'
    shutil.copy2(PROJECT / 'run.csh', entry)
    env['CLAS12_SKIP_SERVER_SYNC'] = '1'
    _, calls = run_script(entry, env, arguments=('--workflow', 'submit', '--config', str(checkout / 'submission.conf')))
    assert len(calls) == 1 and calls[0]['argv'][1] == '--array=1-2'
    _, calls = run_script(entry, env, success=False, arguments=('--workflow', 'submit', '--site', 'removed.json'))
    assert not calls
    # Multiple samples repeat setup with distinct outputs and make exactly one array each.
    new, values, env = fixture(root / 'multiple', 'uniform', '2070MeV')
    first_run = Path(values['OUTPATH'])
    second_run = first_run.with_name('second')
    shutil.copytree(first_run, second_run)
    _, calls = run_script(new, env, arguments=('--lund-dir', str(first_run / 'lundfiles'), '--lund-dir', str(second_run / 'lundfiles')))
    assert len(calls) == 2
    assert [call['env']['OUTPATH'] for call in calls] == [str(first_run), str(second_run)]
    _, calls = run_script(new, env, success=False, arguments=('--lund-dir', str(first_run / 'lundfiles'), '--lund-dir', str(first_run / 'lundfiles')))
    assert not calls  # Resolve every sample before performing any setup or submission.

    # A scheduler rejection stops before the next sample's outputs are replaced.
    (second_run / 'mchipo/keep.hipo').write_text('second sample untouched')
    env['SBATCH_STATUS'] = '1'
    _, calls = run_script(new, env, success=False, arguments=('--lund-dir', str(first_run / 'lundfiles'),
                                                            '--lund-dir', str(second_run / 'lundfiles')))
    assert len(calls) == 1 and (second_run / 'mchipo/keep.hipo').is_file()

    # Clear direct regular logs only once, retaining symlinks, nested logs and fresh scheduler logs.
    new, values, env = fixture(root / 'farm-cleanup', 'uniform', '2070MeV')
    first_run = Path(values['OUTPATH'])
    second_run = first_run.with_name('second')
    shutil.copytree(first_run, second_run)
    farm = new.parent / 'farm_out'
    (farm / 'old.log').write_text('old')
    (farm / 'nested').mkdir()
    (farm / 'nested/keep.log').write_text('keep')
    (farm / 'link.log').symlink_to(farm / 'nested/keep.log')
    scheduler = new.parent / 'bin/sbatch'
    scheduler.write_text(scheduler.read_text().replace('sys.exit(',
        'from pathlib import Path\np=Path(os.environ["FRESH_LOG"])\nassert not p.exists() or p.read_text()=="first"\np.write_text("first" if not p.exists() else "second")\nsys.exit('))
    env['FRESH_LOG'] = str(farm / 'fresh.log')
    _, calls = run_script(new, env, arguments=('--clear-farm-out', 'true', '--lund-dir', str(first_run / 'lundfiles'),
                                              '--lund-dir', str(second_run / 'lundfiles')))
    assert len(calls) == 2 and not (farm / 'old.log').exists()
    assert (farm / 'fresh.log').read_text() == 'second'
    assert (farm / 'link.log').is_symlink() and (farm / 'nested/keep.log').read_text() == 'keep'
    (first_run / 'mchipo/keep.hipo').write_text('keep')
    _, calls = run_script(new, env, success=False, arguments=('--clear-farm-out', 'true', '--farm-out', str(new.parent / 'tags')))
    assert not calls and (first_run / 'mchipo/keep.hipo').is_file()

    # Preloaded data is used unchanged without an override; missing worker programs/data fail before output reset.
    for failure in ('none', 'gemc', 'recon-util', 'sbatch', 'GEMC_DATA_DIR'):
        new, values, env = fixture(root / f'environment-{failure}', 'uniform', '2070MeV')
        config = new.parent / 'submission.conf'
        config.write_text('\n'.join(line for line in config.read_text().splitlines() if not line.startswith('clas12tags-dir')))
        if failure == 'GEMC_DATA_DIR':
            env.pop('GEMC_DATA_DIR')
        elif failure != 'none':
            (new.parent / 'bin' / failure).unlink()
        _, calls = run_script(new, env, success=failure == 'none')
        if failure == 'none':
            assert calls[0]['env']['GEMC_DATA_DIR'] == env['GEMC_DATA_DIR']
        else:
            assert not calls and (Path(values['OUTPATH']) / 'mchipo/old.hipo').exists()

    # Freeze the working C-shell report for both sources and preview/execute modes.
    for source in ('uniform', 'physical'):
        new, values, env = fixture(root / f'report-{source}', source, '2070MeV')
        for execute in (False, True):
            actual, _ = run_script(new, env, execute=execute)
            actual = re.sub(r'(?:\x1b|\\033)\[[0-9;]*m', '', actual).replace(str(new.parent), '{CHECKOUT}')
            mode = 'execute' if execute else 'preview'
            expected = (PROJECT / f'src/slurm-submission/tests/fixtures/{source}-{mode}.txt').read_text()
            assert actual == expected, ''.join(difflib.unified_diff(expected.splitlines(True), actual.splitlines(True)))

    # Execute only the protected worker with inert commands, checking the unchanged detector interface.
    command_log = root / 'detector-commands.jsonl'
    stub = '#!' + sys.executable + '\nimport json,os,sys\nwith open(os.environ["COMMAND_LOG"],"a") as f: f.write(json.dumps([os.path.basename(sys.argv[0]),*sys.argv[1:]])+"\\n")\n'
    for tool in ('gemc', 'recon-util'):
        executable = new.parent / 'bin' / tool
        executable.write_text(stub)
        executable.chmod(0o755)
    _, calls = run_script(new, env)
    worker_env = dict(calls[0]['env'], COMMAND_LOG=str(command_log), SLURM_ARRAY_TASK_ID='2')
    subprocess.run(['bash', str(PAYLOAD)], env=worker_env, check=True, capture_output=True, text=True)
    commands = [json.loads(line) for line in command_log.read_text().splitlines()]
    prefix = values['SAMPLE_FILE_PREFIX']
    out = values['OUTPATH']
    mc = f'{out}/mchipo/mc_{prefix}_2_torus0.5.hipo'
    assert commands == [
        ['gemc', '-USE_GUI=0', '-SCALE_FIELD=binary_torus, 0.5', '-SCALE_FIELD=binary_solenoid, -1.0',
         '-N=3', f'-INPUT_GEN_FILE=lund, {out}/lundfiles/{prefix}_2.txt', '-OUTPUT=hipo, ' + mc, values['GCARD_FILE']],
        ['recon-util', '-y', values['YAML_FILE'], '-n', '3', '-i', mc,
         '-o', f'{out}/reconhipo/recon_{prefix}_2_torus0.5.hipo']]
print('Submission reports, array exports, preview, failure guards and protected worker contract passed.')
# endregion Tests
