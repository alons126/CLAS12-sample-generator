"""Check the sourced SSH launcher without contacting a remote or running GEMC."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

project, shell, build = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])


def sourced(args, cwd=project, env=None, success=True, entry='run.csh'):
    # Values are argv entries to tcsh, not interpolated shell source text.
    program = 'source "$argv[1]" $argv[2-]:q; set result=$status; echo "RESULT=$result"; echo SHELL_ALIVE; /bin/sh -c "exit $result"'
    result = subprocess.run([shell, '-f', '-c', program, str(project/entry), *map(str,args)], cwd=cwd,
                            env=env, text=True, capture_output=True)
    assert ('SHELL_ALIVE' in result.stdout), result.stdout+result.stderr
    assert (result.returncode==0)==success, result.stdout+result.stderr
    return result


with tempfile.TemporaryDirectory(prefix='clas12-launcher-') as tmp:
    root=Path(tmp).resolve()
    output=root/'output with spaces'
    args=['--build','false','--build-dir',build,'--events-per-file','4','--output',output]
    sourced(args)
    m=json.loads((output/'manifest.json').read_text())
    assert m['written_events']==4
    assert m['targets_sha256'] == hashlib.sha256((project/'src/common/targets.h').read_bytes()).hexdigest()
    sourced(args,success=False)  # existing output rejects, but the SSH shell lives
    sourced(['--build','false','--run','false','--jobs','0'],success=False)
    sourced(['--build','false','--run','false','--build-dir',root/'missing-build'])
    sourced(['--build','false','--workflow','genie','--build-dir',build,'--','--help'])
    # From another directory, the documented environment variable identifies the checkout.
    env=dict(os.environ,CLAS12_SAMPLES_DIR=str(project))
    sourced(['--build','false','--run','false'],cwd=root,env=env)
    sourced(['--build','false','--run','false'],entry='scripts/build_and_run.csh')
    # Direct execution also resolves the entry location.
    subprocess.run([str(project/'run.csh'),'--build','false','--run','false'],cwd=root,check=True,capture_output=True)
    # Verify profile/CLI precedence and argv preservation using isolated fake build tools.
    checkout=root/'checkout'
    shutil.copytree(project/'scripts',checkout/'scripts')
    shutil.copytree(project/'config',checkout/'config')
    shutil.copy2(project/'run.csh',checkout/'run.csh')
    binary=root/'fake-bin';binary.mkdir()
    log=root/'build-commands.jsonl'
    cmake=binary/'cmake'
    cmake.write_text('#!'+sys.executable+'\nimport json,os,sys\nwith open(os.environ["BUILD_LOG"],"a") as f: f.write(json.dumps(sys.argv[1:])+"\\n")\n')
    cmake.chmod(0o755)
    env=dict(os.environ,PATH=str(binary)+os.pathsep+os.environ['PATH'],BUILD_LOG=str(log))
    result=subprocess.run([sys.executable,str(checkout/'scripts/workflow.py'),'--run','false','--jobs','2'],env=env,check=True,capture_output=True)
    commands=[json.loads(line) for line in log.read_text().splitlines()]
    assert len(commands)==2 and '--parallel' in commands[1] and commands[1][-1]=='2'
    assert '-DBUILD_TESTING=OFF' in commands[0]
    # Exercise the update helper against an isolated local Git origin.
    remote=root/'origin.git'
    subprocess.run(['git','init','--bare',str(remote)],check=True,capture_output=True)
    for command in [['git','init'],['git','config','user.name','Launcher Test'],['git','config','user.email','launcher@example.invalid'],
                    ['git','add','.'],['git','commit','-m','Fixture'],['git','remote','add','origin',str(remote)],['git','push','-u','origin','HEAD']]:
        subprocess.run(command,cwd=checkout,check=True,capture_output=True)
    update=[sys.executable,str(checkout/'scripts/workflow.py'),'--update-only']
    subprocess.run(update,cwd=checkout,check=True,capture_output=True)
    sentinel=checkout/'local.txt';sentinel.write_text('keep')
    result=subprocess.run(update,cwd=checkout,capture_output=True,text=True)
    assert result.returncode!=0 and sentinel.read_text()=='keep'
    assert 'local changes' in result.stderr
print('Sourced/executed SSH launchers, shell survival, profiles, build commands and safe update passed.')
