#!/bin/tcsh

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
#      tracked server changes, pulls the configured upstream, and prints the resulting HEAD/branch.
#   3. If synchronization succeeds, source the server environment into the caller's shell so its
#      compiler, ROOT, GEMC, reconstruction, and site variables reach the workflow and its children.
#   4. Forward the original quoted argument vector to scripts/workflow.py, which owns configuration,
#      build, tests, LUND-source selection, and ifarm-submission dispatch.
#   5. Restore the caller's directory and return the captured result as both CLAS12_SAMPLE_STATUS and
#      immediate tcsh `$status`, without using `exit` in this normally sourced launcher.
# Usage:
#   source run.csh --workflow create-lund --source uniform [sample options]
#   source run.csh --workflow create-lund --source physical --event-generator genie [sample options]
#   source run.csh --workflow submit [submission options]
# Inputs:
#   $argv carries launcher and child options; CLAS12_SAMPLES_DIR overrides checkout discovery.
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

# An explicit environment value has final precedence. This supports sourcing the launcher from a
# different working directory on ifarm without relying on shell-specific $0 behavior.
if ($?CLAS12_SAMPLES_DIR) then
    set _clas12_root = "$CLAS12_SAMPLES_DIR"
endif

# Convert the selected path to an absolute, normalized directory before any cleanup or Git command.
set _clas12_root = `cd "$_clas12_root" && pwd`

# Require both a Git worktree marker and this project's maintained Python driver. Checking `.git`
# alone could accept an unrelated repository; checking only `workflow.py` could permit destructive
# cleanup in a copied source directory that is not the intended disposable clone.
if (! -d "$_clas12_root/.git" || ! -f "$_clas12_root/scripts/workflow.py") then
    echo "Cannot identify the CLAS12-sample-generator Git checkout: $_clas12_root"

    # Record failure and jump to the shared status-return block. Avoid `exit` because run.csh is
    # normally sourced and must not terminate the user's interactive SSH shell.
    set CLAS12_SAMPLE_STATUS = 1
    goto clas12_launcher_finish
endif
# endregion

# Server mirror update --------------------------------------------------------

# region Server mirror update
# Enter the verified checkout so every relative helper path and every Git command is anchored to
# the intended disposable clone. Suppress pushd's directory-stack printout to keep logs readable.
pushd "$_clas12_root" > /dev/null

# Colors and the logo are presentation helpers. Missing helpers do not block synchronization; the
# updater and Python driver remain responsible for returning the operational status.
if (-f scripts/environment/set_colors.csh) source scripts/environment/set_colors.csh
if (-f scripts/printers/print_logo.csh) source scripts/printers/print_logo.csh

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
# `scripts/code_updater.sh` performs, in order:
#   - `git rev-parse --show-toplevel` to require a recognized worktree;
#   - `git clean -fxd -e build/ -e build` to remove server-only untracked and ignored content while
#     retaining the reusable build tree;
#   - `git reset --hard` to discard server-side tracked edits;
#   - `git pull` to obtain the configured upstream revision; and
#   - `git log -1 --oneline` plus `git branch --show-current` to report the resulting checkout.
# Each state-changing Git command is checked there. Any failure is captured below and prevents the
# environment, build, LUND creation, and job submission stages from running.
if ($_clas12_skip_server_sync == 1) then
    echo "Skipping ifarm checkout replacement (explicit local/test override)."
    set CLAS12_SAMPLE_STATUS = 0
else
    echo "Updating disposable ifarm checkout at $_clas12_root"
    tcsh -f scripts/code_updater.sh
    set CLAS12_SAMPLE_STATUS = $status
endif

# Responsibility 3: load the server environment into this sourced shell.
#
# Source the environment only from the successfully updated checkout. Sourcing is required here so
# compiler, ROOT, GEMC, reconstruction, and site variables remain available to the Python driver and
# its child processes. A setup failure prevents the workflow from running.
if ($CLAS12_SAMPLE_STATUS == 0 && -f scripts/environment/set_environment.csh) then
    source scripts/environment/set_environment.csh
    set CLAS12_SAMPLE_STATUS = $status
endif
# endregion

# Workflow dispatch -----------------------------------------------------------

# region Workflow dispatch
# Responsibility 4: call the maintained Python workflow driver.
#
# Dispatch only after checkout synchronization and environment setup have both succeeded. Quoting
# the argument vector with tcsh's `:q` modifier preserves each user-supplied argument when forwarding
# commands such as `create-lund` or `submit` to the single maintained Python workflow entry point.
if ($CLAS12_SAMPLE_STATUS == 0) then
    python3 scripts/workflow.py $argv:q

    # Capture the driver result immediately, before popd or any later cleanup command can replace
    # `$status`. The caller-status block returns this value to the sourced interactive shell.
    set CLAS12_SAMPLE_STATUS = $status
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
unset _clas12_invocation _clas12_root _clas12_skip_server_sync

# Run a child shell that exits with the captured workflow result. A direct `exit` here would close the
# user's interactive SSH shell because `source` executes this file in that shell. The temporary
# `/bin/sh` exits instead; its code becomes tcsh's immediate `$status` and therefore the status of
# `source run.csh`. CLAS12_SAMPLE_STATUS remains defined for later inspection, whereas `$status` is
# replaced by the caller's next command.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion
