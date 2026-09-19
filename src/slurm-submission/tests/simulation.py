#
# Created by Alon Sportes on 14/09/2026.
#

"""Check the maintained coordinator against the unchanged legacy payload contract.

Purpose:
    Verify previews, metadata, installed discovery, incompatible-input rejection and failures.
Workflow:
    Create a completed run and fake detector binaries; execute only local test fixtures.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

# Command helper ---------------------------------------------------------------
# region Command helper
def call(*args, ok=True):
    """Capture a test command and require the expected process exit status."""
    result = subprocess.run([str(a) for a in args], text=True, capture_output=True)
    assert (result.returncode == 0) == ok, result.stdout + result.stderr
    return result
# endregion

# Test execution ---------------------------------------------------------------
# region Test execution
exe, project = sys.argv[1], Path(sys.argv[2])
with tempfile.TemporaryDirectory(prefix='clas12-simulation-') as tmp:
    root = Path(tmp)
    output_root = root/'run'
    output = output_root/'Uniform_sample_1e_5986MeV'
    call(exe, '--output', output_root, '--events', '25001')
    card = root/'detector.gcard'; card.write_text('<gcard/>')
    yaml = root/'reco.yaml'; yaml.write_text('configuration: test\n')
    runner = project/'src/slurm-submission/run.py'
    payload = project/'src/slurm-submission/external/submit_GEMC_sample.sh'
    options = ['--manifest', output/'lundfiles/lund-gen-monitoring/lund-gen-log.json', '--gcard', card, '--reconstruction', yaml, '--torus', '-1']
    preview = call(sys.executable, runner, *options)
    assert preview.stdout.count('-N=25000') == 1 and preview.stdout.count('-N=1') == 1
    assert ' -n 25000 ' in preview.stdout and ' -n 1 ' in preview.stdout
    assert (output/'mchipo').is_dir() and (output/'reconhipo').is_dir()
    call(sys.executable, runner, *options, '--file-index', '3', ok=False)
    call(sys.executable, runner, *options, '--solenoid', '1', ok=False)
    call(sys.executable, runner, *options, '--output-naming', 'indexed', ok=False)

    # Site paths retain the executable names hardcoded in the external script.
    binaries = root/'bin'; binaries.mkdir()
    gemc = binaries/'gemc'; recon = binaries/'recon-util'
    gemc.write_text('#!'+sys.executable+'\nimport pathlib,sys\np=next(a.split(", ",1)[1] for a in sys.argv if a.startswith("-OUTPUT="))\npathlib.Path(p).write_text("hipo")\n')
    recon.write_text('#!'+sys.executable+'\nimport pathlib,sys\npathlib.Path(sys.argv[sys.argv.index("-o")+1]).write_text("reco")\n')
    gemc.chmod(0o755); recon.chmod(0o755)
    site = root/'site.json'
    site.write_text(json.dumps({'gemc':str(gemc),'recon':str(recon),'slurm':{'account':'clas12','partition':'production','time':'01:00:00','mem':'2G'}}))
    submission = call(sys.executable, project/'src/slurm-submission/submit.py', *options, '--site', site)
    assert '--array=1-2' in submission.stdout and 'submit_GEMC_sample.sh' in submission.stdout
    result = call(sys.executable, runner, *options, '--site', site, '--file-index', '1', '--execute')
    assert 'JOB_GENERATOR = uniform' in result.stdout and 'GEMC_DATA_DIR =' in result.stdout
    record = json.loads((output/'simulation/1.json').read_text())
    assert record['payload_sha256'] == hashlib.sha256(payload.read_bytes()).hexdigest()
    call(sys.executable, runner, *options, '--site', site, '--file-index', '1', '--execute', ok=False)

    # Other generators use the same prefix contract and installed payload location.
    installed = root/'installed'; installed.mkdir()
    for source, name in [(runner,'clas12-simulate'),(project/'src/slurm-submission/submit.py','clas12-submit'),(payload,payload.name)]:
        shutil.copy2(source, installed/name)
    custom = root/'custom'; (custom/'lundfiles/lund-gen-monitoring').mkdir(parents=True)
    (custom/'lundfiles/other_1.txt').write_text('command fixture\n')
    manifest = custom/'lundfiles/lund-gen-monitoring/lund-gen-log.json'
    data = {'schema_version':1,'workflow':'physical','written_events':3,
            'config':{'event-generator':'other-generator','q2-cut':'Q2_0_02'},
            'files':[{'path':'lundfiles/other_1.txt','events':3}]}
    manifest.write_text(json.dumps(data))
    custom_options = ['--manifest',manifest,'--gcard',card,'--reconstruction',yaml,'--torus','0.5','--site',site]
    short_preview = call(sys.executable, installed/'clas12-simulate', *custom_options)
    assert '-N=3' in short_preview.stdout and ' -n 3 ' in short_preview.stdout
    result = call(sys.executable, installed/'clas12-simulate', *custom_options, '--execute')
    assert 'JOB_GENERATOR = other-generator' in result.stdout and 'JOB_Q2_CUT = Q2_0_02' in result.stdout
    assert 'FILE_PREFIX = other' in result.stdout
    assert (custom/'reconhipo/recon_other_1_torus0.5.hipo').is_file()

    # Coordinator uses bash -e without adding error-handling logic to the external script.
    gemc.write_text('#!'+sys.executable+'\nimport sys\nsys.exit(9)\n')
    call(sys.executable, runner, *options, '--site', site, '--file-index', '2', '--execute', ok=False)
    assert (output/'simulation/2.lock').exists()
    assert len(list((output/'reconhipo').glob('*.hipo'))) == 1
print('Legacy payload coordinator integration passed')
# endregion
