#
# Created by Alon Sportes on 14/09/2026.
#

"""Test the launcher when it is sourced or run with tcsh.

Purpose:
    Check quoting, shell survival, settings, build calls, and updates without touching a real server checkout.

Workflow:
    CTest supplies the paths -> temporary fixtures run the commands -> assertions report failures.

Notes:
    Tests use temporary files. External and archived sources stay unchanged.
"""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

project, shell, build = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])

# sourced --------------------------------------------------------------------
# region sourced
def sourced(args, cwd=project, env=None, success=True, entry='run.csh'):
    # Pass values as tcsh arguments so the shell does not rebuild them as source text.
    """Exercise a wrapper inside a sourced tcsh session.

    Algorithm:
        Pass arguments through shell argv, source the wrapper, then check that the shell stays open and returns the expected status.

    Args:
        args: Launcher argument list.
        cwd: Invocation directory.
        env: Optional subprocess environment.
        success: Expected success flag.
        entry: Checkout-relative wrapper path.

    Returns:
        Captured command result. Quoting, shell, or status errors raise an assertion.
    """

    if args:
        program = 'source "$argv[1]" $argv[2-]:q; set result=$status; echo "RESULT=$result"; echo SHELL_ALIVE; /bin/sh -c "exit $result"'
    else:
        # Give the sourced launcher an empty argv, as it would have in an interactive shell.
        program = 'set entry="$argv[1]"; set argv=(); source "$entry"; set result=$status; echo "RESULT=$result"; echo SHELL_ALIVE; /bin/sh -c "exit $result"'

    effective_env = dict(os.environ, CLAS12_SKIP_SERVER_SYNC='1')

    if env:
        effective_env.update(env)

    result = subprocess.run([shell, '-f', '-c', program, str(project/entry), *map(str,args)], cwd=cwd,
                            env=effective_env, text=True, capture_output=True)
    assert ('SHELL_ALIVE' in result.stdout), result.stdout+result.stderr
    assert (result.returncode==0)==success, result.stdout+result.stderr

    return result
# endregion

# Test execution ------------------------------------------------
# region Execution
with tempfile.TemporaryDirectory(prefix='clas12-launcher-') as tmp:
    root=Path(tmp).resolve()
    # Check that a local tcsh variable cannot hide the exported color value after reloading colors.
    color_program = ('set SYSTEM_COLOR = "\\033[35m"; setenv SYSTEM_COLOR "\\033[31m"; '
                     'source "$argv[1]"; set | grep "^SYSTEM_COLOR" >& /dev/null; '
                     'if ($status == 0) exit 1; test "$SYSTEM_COLOR" = "\\033[33m"')

    subprocess.run([shell, '-f', '-c', color_program,
                    str(project/'src/launcher/environment/set_colors.csh')], check=True)

    # Check that an updated helper replaces old color variables before printing its first banner.
    upgrade_env = dict(os.environ, COLOR_START=r'\033[33m', COLOR_END=r'\033[0m')

    for name in ('ERROR_COLOR', 'COMPLETION_COLOR', 'SYSTEM_COLOR', 'INFO_COLOR', 'WARNING_COLOR', 'RESET_COLOR'):
        upgrade_env.pop(name, None)

    upgrade_program = ('source "$argv[1]" >& /dev/null; '
                       'test "$SYSTEM_COLOR" = "\\033[33m" && test "$RESET_COLOR" = "\\033[0m"')

    subprocess.run([shell, '-f', '-c', upgrade_program,
                    str(project/'src/launcher/environment/set_environment.csh')], cwd=project,
                   env=upgrade_env, check=True)

    # Empty and help requests must stop before server synchronization.
    missing=sourced([],success=False)
    assert 'requires an explicit workflow' in missing.stdout
    assert '--workflow create-lund --source uniform' in missing.stdout
    assert '--workflow create-lund --source physical' in missing.stdout
    assert '--workflow submit' in missing.stdout and 'source run.csh --help' in missing.stdout
    assert 'Updating disposable ifarm checkout' not in missing.stdout
    help_result=sourced(['--help'])
    assert '--workflow' in help_result.stdout and 'Choose one of these forms:' in help_result.stdout
    assert 'Updating disposable ifarm checkout' not in help_result.stdout

    # Submission must reject LUND build options before changing build settings.
    submitted=sourced(['--workflow','submit','--run','false'],success=False)
    assert 'unrecognized arguments' in submitted.stderr
    assert 'Updating disposable ifarm checkout' not in submitted.stdout

    output=root/'output with spaces'
    args=['--workflow','create-lund','--source','uniform','--build','false','--build-dir',build,
          '--config','config/samples/uniform-1e-5986MeV.conf','--events','4','--output',output]

    sourced(args)

    output = output/'Uniform_sample_1e_5986MeV'
    m=json.loads((output/'lundfiles/lund-gen-monitoring/lund-gen-log.json').read_text())
    assert m['written_events']==4
    assert m['targets_sha256'] == hashlib.sha256((project/'src/lund-generation/external/targets.h').read_bytes()).hexdigest()
    assert len(m['git']['full_commit_hash']) == 40 and m['git']['repository'].endswith('CLAS12-sample-generator.git')
    sourced(args)  # A second run replaces the same resolved output directory.
    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false','--jobs','0'],success=False)
    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false','--build-dir',root/'missing-build'])
    sourced(['--build','false','--workflow','create-lund','--source','physical','--build-dir',build,'--','--help'])

    # The documented environment variable identifies the checkout from another directory.
    env=dict(os.environ,CLAS12_SAMPLES_DIR=str(project))

    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false'],cwd=root,env=env)
    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false'],entry='src/launcher/build_and_run.csh')
    # Running with tcsh directly also finds the checkout from the script path.
    subprocess.run([shell, str(project/'run.csh'),'--workflow','create-lund','--source','uniform','--build','false','--run','false'],cwd=root,env=dict(os.environ,CLAS12_SKIP_SERVER_SYNC='1'),check=True,capture_output=True)

    # Use a fake CMake command to check setting priority and argument forwarding.
    checkout=root/'checkout'

    shutil.copytree(project/'src/launcher',checkout/'src/launcher')
    shutil.copytree(project/'config',checkout/'config')
    shutil.copy2(project/'run.csh',checkout/'run.csh')

    binary=root/'fake-bin';binary.mkdir()

    log=root/'build-commands.jsonl'
    cmake=binary/'cmake'

    cmake.write_text('#!'+sys.executable+'\nimport json,os,sys\nwith open(os.environ["BUILD_LOG"],"a") as f: f.write(json.dumps(sys.argv[1:])+"\\n")\n')
    cmake.chmod(0o755)

    env=dict(os.environ,PATH=str(binary)+os.pathsep+os.environ['PATH'],BUILD_LOG=str(log))
    result=subprocess.run([sys.executable,str(checkout/'src/launcher/workflow.py'),'--workflow','create-lund','--source','uniform','--run','false','--jobs','2'],env=env,check=True,capture_output=True)
    commands=[json.loads(line) for line in log.read_text().splitlines()]
    assert len(commands)==2 and '--parallel' in commands[1] and commands[1][-1]=='2'
    assert '-DBUILD_TESTING=OFF' in commands[0]

print('Sourced/explicit-tcsh launchers, shell survival, profiles and build commands passed.')

# endregion
