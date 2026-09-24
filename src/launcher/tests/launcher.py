#
# Created by Alon Sportes on 14/09/2026.
#

"""Check sourced and executed SSH launcher behavior.

Purpose:
    Exercise quoting, shell survival, settings, build calls and updates using isolated local fixtures.

Workflow:
    CTest supplies paths and fixtures; assertions or exit codes report failures to the test runner.

Notes:
    Test fixtures are isolated; protected external and legacy sources are read-only.
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
    # Values are argv entries to tcsh, not interpolated shell source text.
    """Exercise a wrapper inside a sourced tcsh session.

    Algorithm:
        Pass arguments through shell argv, source the wrapper, then check shell survival and exit status.

    Args:
        args: Launcher argument list.
        cwd: Invocation directory.
        env: Optional subprocess environment.
        success: Expected success flag.
        entry: Checkout-relative wrapper path.

    Returns:
        Captured result; quoting, shell-exit or status mismatches raise an assertion.
    """

    if args:
        program = 'source "$argv[1]" $argv[2-]:q; set result=$status; echo "RESULT=$result"; echo SHELL_ALIVE; /bin/sh -c "exit $result"'
    else:
        # Model an interactive tcsh, whose argv is empty, rather than leaking this test process's
        # entry-path argument into the sourced launcher's no-argument preflight.
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
    # Reproduce the tcsh failure mode where local variables shadow exported values. The shared
    # loader must clear both namespaces and restore the canonical palette in a sourced shell.
    color_program = ('set SYSTEM_COLOR = "\\033[35m"; setenv SYSTEM_COLOR "\\033[31m"; '
                     'source "$argv[1]"; set | grep "^SYSTEM_COLOR" >& /dev/null; '
                     'if ($status == 0) exit 1; test "$SYSTEM_COLOR" = "\\033[33m"')

    subprocess.run([shell, '-f', '-c', color_program,
                    str(project/'src/launcher/environment/set_colors.csh')], check=True)

    # Model an in-place update whose parent shell still contains the retired palette names. The
    # newly pulled environment helper must load its current palette before printing its first banner.
    upgrade_env = dict(os.environ, COLOR_START=r'\033[33m', COLOR_END=r'\033[0m')

    for name in ('ERROR_COLOR', 'COMPLETION_COLOR', 'SYSTEM_COLOR', 'INFO_COLOR', 'WARNING_COLOR', 'RESET_COLOR'):
        upgrade_env.pop(name, None)

    upgrade_program = ('source "$argv[1]" >& /dev/null; '
                       'test "$SYSTEM_COLOR" = "\\033[33m" && test "$RESET_COLOR" = "\\033[0m"')

    subprocess.run([shell, '-f', '-c', upgrade_program,
                    str(project/'src/launcher/environment/set_environment.csh')], cwd=project,
                   env=upgrade_env, check=True)

    # Empty/help invocations are resolved before destructive server synchronization.
    missing=sourced([],success=False)
    assert 'requires an explicit workflow' in missing.stdout
    assert '--workflow create-lund --source uniform' in missing.stdout
    assert '--workflow create-lund --source physical' in missing.stdout
    assert '--workflow submit' in missing.stdout and 'source run.csh --help' in missing.stdout
    assert 'Updating disposable ifarm checkout' not in missing.stdout
    help_result=sourced(['--help'])
    assert '--workflow' in help_result.stdout and 'Choose one of these forms:' in help_result.stdout
    assert 'Updating disposable ifarm checkout' not in help_result.stdout

    # Submission no longer accepts LUND build flags or touches build settings.
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
    sourced(args)  # legacy behavior replaces the resolved run directory
    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false','--jobs','0'],success=False)
    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false','--build-dir',root/'missing-build'])
    sourced(['--build','false','--workflow','create-lund','--source','physical','--build-dir',build,'--','--help'])

    # From another directory, the documented environment variable identifies the checkout.
    env=dict(os.environ,CLAS12_SAMPLES_DIR=str(project))

    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false'],cwd=root,env=env)
    sourced(['--workflow','create-lund','--source','uniform','--build','false','--run','false'],entry='src/launcher/build_and_run.csh')
    # Explicit tcsh execution also resolves the entry location. The ownership header precedes the interpreter directive,
    # so callers that do not source this workflow must select tcsh themselves.
    subprocess.run([shell, str(project/'run.csh'),'--workflow','create-lund','--source','uniform','--build','false','--run','false'],cwd=root,env=dict(os.environ,CLAS12_SKIP_SERVER_SYNC='1'),check=True,capture_output=True)

    # Verify profile/CLI precedence and argv preservation using isolated fake build tools.
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
