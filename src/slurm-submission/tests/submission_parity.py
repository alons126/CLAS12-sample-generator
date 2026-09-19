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
def region(text, name, replacement):
    """Replace one named editable region in a temporary script, preserving its markers."""
    return re.sub(r'(?m)(^[ \t]*# region ' + re.escape(name) + r'\n).*?(^[ \t]*# endregion ' + re.escape(name) + r'$)',
                  lambda match: match[1] + replacement + '\n' + match[2], text, flags=re.S)


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
    common = f'''set samples = ( example )
setenv CLEAR_FAR_OUT false
setenv CUSTOM_GEMC_VERSION true
setenv GEMC_VERSION 5.14
setenv CLAS12TAGS_DIR {root}/tags
set farm_out = {root}/farm_out'''
    settings = f'set source = {source}\n' + '\n'.join(f'setenv {name} "{value}"' for name, value in values.items())
    new = region(region(SETUP.read_text(), 'Settings', common), 'Sample settings', settings)
    new_path = root / 'new.csh'
    new_path.write_text(new)
    env = dict(os.environ, PATH=str(bin_dir) + os.pathsep + os.environ['PATH'], MODULE_LOG=str(root / 'modules.log'),
               SBATCH_LOG=str(root / 'sbatch.jsonl'), FIXTURE_GEMC_DATA=str(root / 'gemc-data'),
               MODULE_STATUS='0', GEMC_DATA_DIR='/wrong/inherited/value', SBATCH_STATUS='0')
    return new_path, old_path, values, env


def run_script(path, env, success=True, arguments=()):
    """Source a fixture with a shell module alias; verify status and caller-shell survival."""
    for key in ('MODULE_LOG', 'SBATCH_LOG'):
        Path(env[key]).unlink(missing_ok=True)
    program = 'alias module \'source "' + str(path.parent / 'module.csh') + '" \\!*\'; source "$argv[1]" $argv[2-]:q; set result=$status; echo "SHELL_ALIVE=$result"; /bin/sh -c "exit $result"'
    result = subprocess.run([SHELL, '-f', '-c', program, str(path), *arguments], cwd=path.parent, env=env, capture_output=True, text=True)
    assert f'SHELL_ALIVE={result.returncode}\n' in result.stdout, result.stdout + result.stderr
    assert (result.returncode == 0) == success, result.stdout + result.stderr
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
            new.write_text(new.read_text().replace(f'setenv OUTPATH "{run}"', 'setenv OUTPATH "/"'))
        elif failure == 'sbatch':
            env['SBATCH_STATUS'] = '1'
        _, calls = run_script(new, env, success=False)
        assert len(calls) == (1 if failure == 'sbatch' else 0)
        if failure != 'sbatch':
            assert (run / 'reconhipo/old.hipo').exists()
    # Exercise the actual editable examples and beam switch, not just relocated legacy settings.
    for source in ('uniform', 'physical'):
        new, _, values, env = fixture(root / f'configured-{source}', source, '2070MeV')
        checkout = new.parent
        configured = region(SETUP.read_text(), 'Settings', f"""set samples = ( {source}-example )
setenv CLEAR_FAR_OUT false
setenv CUSTOM_GEMC_VERSION true
setenv GEMC_VERSION 5.14
setenv CLAS12TAGS_DIR {checkout}/tags
set farm_out = {checkout}/farm_out""")
        configured = re.sub(r'(?m)^(\s*)setenv OUTPATH_BASE .*$', lambda m: m[1] + f'setenv OUTPATH_BASE {checkout}/output', configured)
        configured = configured.replace('setenv NUM_OF_JOBS 5000', 'setenv NUM_OF_JOBS 2')
        configured = re.sub(r'setenv JOB_NEVENTS (25000|10000)', 'setenv JOB_NEVENTS 3', configured)
        new.write_text(configured)
        if source == 'uniform':
            name, prefix, target = 'Uniform_sample_enFD_2070MeV', 'Uniform_sample_enFD_2070MeV', 'rgm_fall2021_Ar'
        else:
            name = 'rgm_fall2021_C_S__genie-none__GEM21_11a_00_000__Q2_0_02__2070MeV_GEMC-5.14'
            prefix, target = 'C12_genie_2070MeV', 'rgm_fall2021_C_S'
        configured_run = checkout / 'output' / name
        shutil.copytree(Path(values['OUTPATH']), configured_run)
        for index in (1, 2):
            (configured_run / f'lundfiles/{values["SAMPLE_FILE_PREFIX"]}_{index}.txt').rename(configured_run / f'lundfiles/{prefix}_{index}.txt')
        resources = checkout / 'config/detector/Generation_files_2GeV/5.14'
        resources.mkdir(parents=True)
        # Fixtures are in a temporary checkout, never the protected repository paths.
        (resources / f'{target}_2GeV.gcard').write_text('<gcard/>')
        (resources / 'rgm_fall2021-cv.yaml').write_text('test: true\n')
        _, calls = run_script(new, env)
        assert len(calls) == 1
        assert calls[0]['env']['OUTPATH'] == str(configured_run)
        assert calls[0]['env']['SAMPLE_FILE_PREFIX'] == prefix
        assert calls[0]['env']['GCARD_FILE'] == str(resources / f'{target}_2GeV.gcard')

    # Exercise the real run.csh branch without sync or the Python/build machinery.
    new, _, values, env = fixture(root / 'launcher', 'uniform', '2070MeV')
    checkout = new.parent
    (checkout / '.git').mkdir()
    (checkout / 'src/launcher').mkdir()
    (checkout / 'src/launcher/workflow.py').write_text('raise AssertionError("Submission must bypass Python")\n')
    shutil.copy2(new, checkout / 'src/slurm-submission/setup_and_submit.csh')
    entry = checkout / 'run.csh'
    shutil.copy2(PROJECT / 'run.csh', entry)
    env['CLAS12_SKIP_SERVER_SYNC'] = '1'
    _, calls = run_script(entry, env, arguments=('--workflow', 'submit'))
    assert len(calls) == 1 and calls[0]['argv'][1] == '--array=1-2'
    _, calls = run_script(entry, env, success=False, arguments=('--workflow', 'submit', '--site', 'removed.json'))
    assert not calls
    env['MODULE_STATUS'] = '1'
    _, calls = run_script(entry, env, success=False, arguments=('--workflow', 'submit'))
    assert not calls

    # Multiple samples repeat setup with distinct outputs and make exactly one array each.
    new, _, values, env = fixture(root / 'multiple', 'uniform', '2070MeV')
    first_run = Path(values['OUTPATH'])
    second_run = first_run.with_name('second')
    shutil.copytree(first_run, second_run)
    text = new.read_text().replace('set samples = ( example )', 'set samples = ( example second )')
    text = text.replace('    # endregion Sample settings',
        f'if ("$sample" == "second") setenv OUTPATH "{second_run}"\n    # endregion Sample settings')
    new.write_text(text)
    _, calls = run_script(new, env)
    assert len(calls) == 2
    assert [call['env']['OUTPATH'] for call in calls] == [str(first_run), str(second_run)]
    # Do not reset a directory again after submitting an array that writes into it.
    new.write_text(text.replace(f'if ("$sample" == "second") setenv OUTPATH "{second_run}"', ''))
    _, calls = run_script(new, env, success=False)
    assert len(calls) == 1

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
