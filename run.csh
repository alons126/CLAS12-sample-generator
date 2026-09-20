#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# run.csh --------------------------------------------------------------------
# Description:
#   Authoritative ifarm checkout entry point.
# Purpose:
#   Replace the disposable server clone with the remote revision, load its environment, then build
#   and run one configured CLAS12 sample workflow.
# Workflow:
#   1. Find the checkout from the caller's directory, this file, or CLAS12_SAMPLES_DIR; normalize
#      it and verify both `.git` and the project workflow driver before any destructive command.
#   2. Enter that verified checkout and run the intentionally destructive updater in a child tcsh.
#      The updater validates the Git worktree, cleans untracked/ignored files except build/, resets
#      tracked server changes, pulls the configured upstream, initializes pinned submodules, and
#      prints the resulting HEAD/branch.
#   3. After synchronization, initialize the LUND environment or let the sourced submission script
#      load its GEMC module in the login shell, retaining that environment for sbatch.
#   4. Source the single submission script for submit; otherwise forward the original quoted
#      arguments to src/launcher/workflow.py for building, testing and LUND creation.
#   5. Restore the caller's directory and return the captured result as both CLAS12_SAMPLE_STATUS and
#      immediate tcsh `$status`, without using `exit` in this normally sourced launcher.
# Usage:
#   source run.csh --workflow create-lund --source uniform \
#     --config config/samples/uniform-1e-5986MeV.conf --output OUTPUT_PARENT
#   source run.csh --workflow create-lund --source physical \
#     --config config/samples/genie.conf --input 'GST_GLOB' --output OUTPUT_PARENT
#   source run.csh --workflow submit --lund-dir RUN/lundfiles [overrides]
# Inputs:
#   $argv carries launcher and child options. CLAS12_SAMPLES_DIR is an optional environment variable
#   set by the user with `setenv`; when present, it supplies the absolute checkout path and overrides
#   automatic discovery. The project does not create this variable.
# Outputs:
#   CLAS12_SAMPLE_STATUS and immediate $status report the complete update/build/run result.
# Notes:
#   The ifarm checkout is disposable. Commit and push valuable changes from the local development
#   clone before sourcing this file. The updater resets tracked changes and cleans untracked files.

# Checkout discovery ----------------------------------------------------------

# region Checkout discovery
# Responsibility 1: find and verify the repository.
#
# In tcsh, $0 identifies the script when run directly, but commonly identifies the parent shell
# (`tcsh`, `csh`, or an option-like value) when this file is sourced. Start from the caller's current
# directory so the documented `source run.csh` command works from the checkout root.
set _clas12_invocation = "$0"
set _clas12_root = "$cwd"

# During direct execution, derive the checkout from the script's containing directory. The `:t`
# modifier extracts the final path component and `:h` extracts the parent directory.
if ("$_clas12_invocation:t" != "tcsh" && "$_clas12_invocation:t" != "csh" && "$_clas12_invocation" !~ "-*") then
    set _clas12_root = "$_clas12_invocation:h"
endif

# An explicit environment value has final precedence. CLAS12_SAMPLES_DIR is not defined by this
# project: the user sets it only when they want to source run.csh from outside the checkout, for example:
#   setenv CLAS12_SAMPLES_DIR /shared/path/CLAS12-sample-generator
#   source "$CLAS12_SAMPLES_DIR/run.csh"
# It remains in the shell until `unsetenv CLAS12_SAMPLES_DIR`. When the user is already at the checkout
# root, `source run.csh` uses $cwd and the variable is unnecessary.
if ($?CLAS12_SAMPLES_DIR) then
    set _clas12_root = "$CLAS12_SAMPLES_DIR"
endif

# Convert the selected path to an absolute, normalized directory before any cleanup or Git command.
set _clas12_root = `cd "$_clas12_root" && pwd`

# Require both a Git worktree marker and this project's maintained Python driver. Checking `.git`
# alone could accept an unrelated repository; checking only `workflow.py` could permit destructive
# cleanup in a copied source directory that is not the intended disposable clone.
if (! -d "$_clas12_root/.git" || ! -f "$_clas12_root/src/launcher/workflow.py") then
    echo "Cannot identify the CLAS12-sample-generator Git checkout: $_clas12_root"

    # Record failure and jump to the shared status-return block. Avoid `exit` because run.csh is
    # normally sourced and must not terminate the user's interactive SSH shell.
    set CLAS12_SAMPLE_STATUS = 1
    goto clas12_launcher_finish
endif

# Validate the two information-only/incomplete forms before entering the checkout or changing it.
# `--help` is handled directly by the maintained Python parser. An empty invocation prints concrete
# commands and returns usage status 2. Neither path performs clean/reset/pull, loads the environment,
# configures CMake, creates LUND files, or submits jobs.
if ($#argv == 1) then
    if ("$argv[1]" == "--help") then
        python3 "$_clas12_root/src/launcher/workflow.py" --help
        set CLAS12_SAMPLE_STATUS = $status
        goto clas12_launcher_finish
    endif
endif

if ($#argv == 0) then
    echo "Error: source run.csh requires an explicit workflow."
    echo ""
    echo "Create a uniform LUND sample:"
    echo '  source run.csh --workflow create-lund --source uniform \'
    echo "    --config config/samples/uniform-1e-5986MeV.conf --output OUTPUT_PARENT"
    echo ""
    echo "Convert physical generator output:"
    echo '  source run.csh --workflow create-lund --source physical \'
    echo "    --config config/samples/genie.conf --input 'GST_GLOB' --output OUTPUT_PARENT"
    echo ""
    echo "Submit completed LUND files:"
    echo "  source run.csh --workflow submit --lund-dir RUN/lundfiles"
    echo ""
    echo "Build and test without running a workflow payload:"
    echo "  source run.csh --workflow create-lund --source uniform --build true --test true --run false"
    echo ""
    echo "Run 'source run.csh --help' for launcher options."
    echo "After selecting a create-lund source, add '-- --help' for its sample options."
    set CLAS12_SAMPLE_STATUS = 2
    goto clas12_launcher_finish
endif
# endregion

# Submission selection -------------------------------------------------------

# region Submission selection
# Submission accepts manifest/config/CLI inputs; its resolver has no build or submission responsibilities.
set _clas12_submit = 0
if ($#argv >= 1) then
    if ("$argv[1]" == "--workflow=submit") set _clas12_submit = 1
endif
if ($#argv >= 2) then
    if ("$argv[1]" == "--workflow" && "$argv[2]" == "submit") set _clas12_submit = 1
endif
if ($_clas12_submit == 1) then
    set _clas12_submission_args = ($argv:q)
    if ("$argv[1]" == "--workflow=submit") then
        shift _clas12_submission_args
    else
        shift _clas12_submission_args
        shift _clas12_submission_args
    endif
    # Help and malformed arguments must not trigger the destructive server refresh.
    python3 "$_clas12_root/src/slurm-submission/resolve_inputs.py" --check-arguments $_clas12_submission_args:q
    set CLAS12_SAMPLE_STATUS = $status
    if ($CLAS12_SAMPLE_STATUS != 0) goto clas12_launcher_finish
    foreach _clas12_argument ($_clas12_submission_args:q)
        if ("$_clas12_argument" == "--help") goto clas12_launcher_finish
    end
endif
# endregion Submission selection

# Server mirror update --------------------------------------------------------

# region Server mirror update
# Enter the verified checkout so every relative helper path and every Git command is anchored to
# the intended disposable clone. Suppress pushd's directory-stack printout to keep logs readable.
pushd "$_clas12_root" > /dev/null

# Colors and the logo are presentation helpers. Missing helpers do not block synchronization; the
# updater and Python driver remain responsible for returning the operational status.
if (-f src/launcher/environment/set_colors.csh) source src/launcher/environment/set_colors.csh
if (-f src/launcher/printers/print_logo.csh) source src/launcher/printers/print_logo.csh

# Resolve the local/test bypass without expanding an undefined tcsh variable. Normal ifarm use
# leaves CLAS12_SKIP_SERVER_SYNC unset, so synchronization remains the default behavior.
set _clas12_skip_server_sync = 0
if ($?CLAS12_SKIP_SERVER_SYNC) then
    if ("$CLAS12_SKIP_SERVER_SYNC" == "1") set _clas12_skip_server_sync = 1
endif

# Responsibility 2: replace the disposable ifarm clone with the pushed repository state.
#
# Automated launcher tests and deliberate local debugging may bypass destructive Git operations.
# Otherwise run the updater in a child tcsh: its `exit` calls cannot terminate the sourced parent
# shell, and its exit status becomes the gate for environment setup and workflow execution.
# `src/launcher/code_updater.sh` performs, in order:
#   - `git rev-parse --show-toplevel` to require a recognized worktree;
#   - `git clean -fxd -e build/ -e build` to remove server-only untracked and ignored content while
#     retaining the reusable build tree;
#   - `git reset --hard` to discard server-side tracked edits;
#   - `git pull` to obtain the configured upstream revision;
#   - `git submodule sync --recursive` and `git submodule update --init --recursive` to check out
#     the exact external reference revisions recorded by that revision; and
#   - `git log -1 --oneline` plus `git branch --show-current` to report the resulting checkout.
# Each state-changing Git command is checked there. Any failure is captured below and prevents the
# environment, build, LUND creation, and job submission stages from running.
if ($_clas12_skip_server_sync == 1) then
    echo "Skipping ifarm checkout replacement (explicit local/test override)."
    set CLAS12_SAMPLE_STATUS = 0
else
    echo "Updating disposable ifarm checkout at $_clas12_root"
    tcsh -f src/launcher/code_updater.sh
    set CLAS12_SAMPLE_STATUS = $status
endif

# Responsibility 3: load the server environment into this sourced shell.
#
# Source the environment only from the successfully updated checkout. Sourcing is required here so
# compiler, ROOT, GEMC, reconstruction, and site variables remain available to the Python driver and
# its child processes. A setup failure prevents the workflow from running.
if ($CLAS12_SAMPLE_STATUS == 0 && $_clas12_submit == 0 && -f src/launcher/environment/set_environment.csh) then
    source src/launcher/environment/set_environment.csh
    set CLAS12_SAMPLE_STATUS = $status
endif
# endregion

# Workflow dispatch -----------------------------------------------------------

# region Workflow dispatch
# Submission is sourced in the login shell so module aliases and exported settings reach sbatch.
# LUND creation retains its existing Python build/creation driver.
if ($CLAS12_SAMPLE_STATUS == 0) then
    if ($_clas12_submit == 1) then
        source src/slurm-submission/setup_and_submit.csh $_clas12_submission_args:q
        set CLAS12_SAMPLE_STATUS = $status
    else
        python3 src/launcher/workflow.py $argv:q
        set CLAS12_SAMPLE_STATUS = $status
    endif
endif

# Balance the earlier pushd for every path that reaches this block and restore the directory from
# which the user sourced run.csh. Suppress tcsh's directory-stack output because it is not a workflow
# result; popd does not replace the already captured workflow status.
popd > /dev/null
# endregion

# Caller status ---------------------------------------------------------------

# region Caller status
# Responsibility 5: return status without closing the sourced interactive shell.
#
# Every completion path converges here. In particular, checkout validation uses `goto` to reach this
# block before any directory change, while the normal path arrives here after restoring the caller's
# working directory.
clas12_launcher_finish:

# Remove launcher-only scratch variables from the interactive environment because sourcing executes
# this file in the caller's tcsh process. Keep CLAS12_SAMPLE_STATUS available so the user can inspect
# the named workflow result after control returns.
unset _clas12_invocation _clas12_root
if ($?_clas12_submit) unset _clas12_submit
if ($?_clas12_submission_args) unset _clas12_submission_args
if ($?_clas12_argument) unset _clas12_argument
if ($?_clas12_skip_server_sync) unset _clas12_skip_server_sync

# Run a child shell that exits with the captured workflow result. A direct `exit` here would close the
# user's interactive SSH shell because `source` executes this file in that shell. The temporary
# `/bin/sh` exits instead; its code becomes tcsh's immediate `$status` and therefore the status of
# `source run.csh`. CLAS12_SAMPLE_STATUS remains defined for later inspection, whereas `$status` is
# replaced by the caller's next command.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion
