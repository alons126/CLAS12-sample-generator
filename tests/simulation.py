"""Validate dry runs, exact event counts, Slurm planning and failure propagation."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def call(*args, ok=True):
    r = subprocess.run([str(a) for a in args], text=True, capture_output=True)
    assert (r.returncode==0)==ok, r.stdout+r.stderr
    return r


exe, project = sys.argv[1], Path(sys.argv[2])
with tempfile.TemporaryDirectory(prefix='clas12-simulation-') as tmp:
    root=Path(tmp)
    output=root/'run with spaces'
    call(exe,'--output',output,'--events-per-file','7','--files','2')
    card=root/'detector.gcard'; card.write_text('<gcard/>')
    yaml=root/'reco.yaml'; yaml.write_text('configuration: test\n')
    runner=project/'scripts/simulation/run.py'
    options=['--manifest',output/'manifest.json','--gcard',card,'--reconstruction',yaml,'--torus','-1']
    result=call(sys.executable,runner,*options)
    assert result.stdout.count('-N=7')==2 and result.stdout.count('-n 7')==2
    assert not (output/'mchipo').exists()
    call(sys.executable,runner,*options,'--file-index','3',ok=False)
    # Executable stubs verify command argument boundaries and the successful chain.
    gemc=root/'fake gemc'
    gemc.write_text('#!'+sys.executable+'\nimport pathlib,sys\np=next(a.split(", ",1)[1] for a in sys.argv if a.startswith("-OUTPUT="))\npathlib.Path(p).write_text("hipo")\n')
    gemc.chmod(0o755)
    recon=root/'fake recon'
    recon.write_text('#!'+sys.executable+'\nimport pathlib,sys\npathlib.Path(sys.argv[sys.argv.index("-o")+1]).write_text("reco")\n')
    recon.chmod(0o755)
    site=root/'site.json'
    site.write_text(json.dumps({'gemc':str(gemc),'recon':str(recon),'slurm':{'account':'clas12','partition':'production','time':'01:00:00','mem':'2G'}}))
    result=call(sys.executable,project/'scripts/slurm/submit.py',*options,'--site',site)
    assert '--array=1-2' in result.stdout and 'SLURM_ARRAY_TASK_ID' in result.stdout
    call(sys.executable,runner,*options,'--site',site,'--file-index','1','--execute')
    assert (output/'simulation/1.json').exists()
    assert (output/'reconhipo/recon_1.hipo').read_text()=='reco'
    call(sys.executable,runner,*options,'--site',site,'--file-index','1','--execute',ok=False)
    gemc.write_text('#!'+sys.executable+'\nimport sys\nsys.exit(9)\n')
    call(sys.executable,runner,*options,'--site',site,'--file-index','2','--execute',ok=False)
    assert not (output/'reconhipo/recon_2.hipo').exists()
    assert (output/'simulation/2.lock').exists()
print('simulation integration passed')
