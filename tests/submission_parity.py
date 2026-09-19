#
# Created by Alon Sportes on 14/09/2026.
#

"""Compare archived and current detector-job arguments.

Purpose:
    Run historical payloads with fake executables in temporary directories; never submit real jobs.

Workflow:
    CTest supplies paths and fixtures; assertions or exit codes report failures to the test runner.

Notes:
    Test fixtures are isolated; protected external and legacy sources are read-only.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

project=Path(sys.argv[1])
legacy_payload=(project/'legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh').read_bytes()
unified_payload=(project/'src/common/external/submit_GEMC_sample.sh').read_bytes()
assert unified_payload.split(b'JOB_TARGET=')[0] == legacy_payload.split(b'JOB_TARGET=')[0]
generalized_tail = unified_payload[unified_payload.index(b'NEVENTS=${JOB_NEVENTS:?JOB_NEVENTS is required}'):]
generalized_tail = generalized_tail.replace(b'NEVENTS=${JOB_NEVENTS:?JOB_NEVENTS is required}', b'NEVENTS=10000', 1)
assert generalized_tail.split(b'#set output file path location', 1)[1] == legacy_payload.split(b'#set output file path location', 1)[1]
assert generalized_tail.startswith(b'NEVENTS=10000\n')
assert b'TORUS=${TORUS_FIELD}' in generalized_tail

# Test execution ------------------------------------------------
# region Execution
with tempfile.TemporaryDirectory(prefix='clas12-job-parity-') as tmp:
    root=Path(tmp).resolve()
    fake=root/'bin';fake.mkdir()
    stub='''#!'''+sys.executable+'''
import json, os, pathlib, sys
with open(os.environ['COMMAND_LOG'], 'a') as log:
    log.write(json.dumps([pathlib.Path(sys.argv[0]).name, *sys.argv[1:]])+'\\n')
if pathlib.Path(sys.argv[0]).name=='gemc':
    path=next(a.split(', ',1)[1] for a in sys.argv if a.startswith('-OUTPUT='))
else:
    path=sys.argv[sys.argv.index('-o')+1]
pathlib.Path(path).write_text('stub output')
'''
    for binary in ['gemc','recon-util']:
        p=fake/binary;p.write_text(stub);p.chmod(0o755)
    card=root/'detector.gcard';card.write_text('<gcard/>')
    reco=root/'reco.yaml';reco.write_text('test: true\n')
    for workflow in ['uniform','GENIE']:
        for label,torus in [('2070MeV','0.5'),('4029MeV','-1.0'),('5986MeV','-1.0')]:
            prefix=f'Uniform_en_sample_{label}' if workflow=='uniform' else f'C12_GEM21_11a_00_000_Q2_0_02_{label}'
            logs=[]
            for implementation in ['old','new']:
                run=root/f'{workflow}-{label}-{implementation}';run.mkdir()
                for folder in ['lundfiles/lund-gen-monitoring','mchipo','reconhipo']:(run/folder).mkdir(parents=True)
                (run/'lundfiles'/f'{prefix}_1.txt').write_text('command-contract fixture\n')
                (run/'lundfiles/lund-gen-monitoring/lund-gen-log.json').write_text(json.dumps({'schema_version':1,'written_events':10000,'files':[{'path':f'lundfiles/{prefix}_1.txt','events':10000}]}))
                log=run/'commands.jsonl'
                env=dict(os.environ,PATH=str(fake)+os.pathsep+os.environ['PATH'],COMMAND_LOG=str(log),
                         TEMP_OUTPATH_PARTICLE='en',TEMP_BEAM_E=label,TORUS_FIELD=torus,OUTPATH=str(run),GCARD_FILE=str(card),YAML_FILE=str(reco),
                         SLURM_ARRAY_TASK_ID='1',SAMPLE_TARGET_NUCLEUS='C12',GENIE_TUNE='GEM21_11a_00_000',Q2_CUT='Q2_0_02')
                if implementation=='old':
                    cmd=['bash',str(project/f'legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_{workflow}_sample.sh')]
                else:
                    cmd=[sys.executable,str(project/'scripts/simulation/run.py'),'--manifest',str(run/'lundfiles/lund-gen-monitoring/lund-gen-log.json'),'--gcard',str(card),'--reconstruction',str(reco),'--torus',torus,'--execute']
                result=subprocess.run(cmd,env=env,capture_output=True,text=True)
                assert result.returncode==0,result.stdout+result.stderr
                if implementation=='new':
                    assert 'JOB_TARGET = C12' in result.stdout and 'GEMC_DATA_DIR =' in result.stdout
                    assert 'JOB_GENERATOR_TUNE = GEM21_11a_00_000' in result.stdout
                    assert f'JOB_BEAM_E = {label}' in result.stdout
                logs.append([[arg.replace(str(run),'<RUN>') for arg in json.loads(line)] for line in log.read_text().splitlines()])
            assert logs[0]==logs[1],(workflow,label,logs)
print('Legacy/new GEMC and reconstruction argv match for both workflows and all three energies.')

# endregion
