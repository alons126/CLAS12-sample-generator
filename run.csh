#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# run.csh --------------------------------------------------------------------
# Description:
#   Main ifarm entry point.
# 
# Purpose:
#   Update the disposable server checkout, load its environment, and run one workflow.
#
# Workflow:
#   1. Find and verify the checkout before changing any files.
#   2. Replace the ifarm checkout with the remote version, while keeping the build directory.
#   3. Load the needed software environment.
#   4. Create LUND files or submit simulation jobs.
#   5. Restore the caller's directory and return the workflow status without closing the shell.
# 
# CLI options (launcher-owned for create-lund):
#   --workflow create-lund|submit  Select one of the two user-facing workflows (required).
#   --source uniform|physical      Select event content for create-lund; submit has its own
#                                  forwarded --source option for manifest-free inputs.
#   --run-settings FILE            Select strict build JSON (default: config/run.json).
#   --build true|false             Configure and build LUND applications (default from run JSON).
#   --run true|false               Execute the selected LUND application (default from run JSON).
#   --build-dir DIRECTORY          Select the CMake binary tree.
#   --build-type TYPE              Select Debug, Release, RelWithDebInfo, or MinSizeRel.
#   --jobs N                       Select positive parallel build workers.
#   --help                         Print launcher help without synchronizing the ifarm checkout.
#
# CLI options (forwarded to submit.py for submit):
#   --lund-dir DIRECTORY           Select completed LUND input; repeat for multiple samples.
#   --config FILE                  Read optional key = value submission settings.
#   --execute                      Replace simulation outputs and submit; default is preview.
#   --source uniform|physical      Override source metadata when no manifest supplies it.
#   --beam-energy GeV              Set truth beam energy; normally read from the manifest.
#   --rgm-target ID                Set truth target identity; normally read from the manifest.
#   --channel NAME                 Set uniform channel; normally read from the manifest.
#   --hadron NAME                  Set uniform hadron when channel=eh.
#   --hadron-region FD|CD          Set uniform hadron region when channel=eh.
#   --event-generator NAME         Set physical input adapter; default: genie-gst without a manifest.
#   --tune NAME                    Set physical tune; default: unknown without a manifest.
#   --q2-cut NAME                  Set physical Q2 label; no cut is applied here.
#   --prefix NAME                  Set LUND filename prefix; required without a manifest.
#   --gemc-version VERSION         Select GEMC resources; fallback default: 5.14.
#   --gemc-target-variation NAME   Select detector target variation.
#   --gcard FILE / --yaml FILE     Override detector and reconstruction inputs.
#   --torus SCALE                  Override the beam-dependent torus default.
#   --num-jobs N                   Submit first N completed LUND files; default: all.
#   --events-per-job N             Set common event limit; default: selected-file maximum.
#   --job-name NAME                Override the metadata-derived Slurm job name.
#   --clas12tags-dir DIRECTORY     Use a custom clas12Tags checkout as GEMC_DATA_DIR.
#   --clear-farm-out true|false    Clear direct farm log files with --execute; default: false.
#   --farm-out DIRECTORY           Set farm_out path when clearing it.
#   --fc-status 0|1                Set legacy physical report/filename label; default: 0.
# 
# Usage:
#   source run.csh --workflow create-lund --source uniform \
#     --config config/samples/uniform-1e-5986MeV.conf --output OUTPUT_PARENT
#   source run.csh --workflow create-lund --source physical \
#     --config config/samples/genie-gst.conf --input 'GST_GLOB' --output OUTPUT_PARENT
#   source run.csh --workflow submit --lund-dir RUN/lundfiles [overrides]
# 
# Forwarded options:
#   create-lund passes remaining options to the selected LUND program. Submit passes them to submit.py.
# 
# Inputs:
#   $argv contains launcher and workflow options. An optional CLAS12_SAMPLES_DIR value sets the checkout.
# 
# Outputs:
#   CLAS12_SAMPLE_STATUS and $status contain the final result.
# 
# Notes:
#   The ifarm checkout is disposable. Commit and push valuable changes from the local development
#   clone before sourcing this file. The updater resets tracked changes and cleans untracked files.

# Checkout discovery ----------------------------------------------------------

# region Checkout discovery
# When sourced, $0 often names the parent shell. Start from the current directory.
set _clas12_invocation = "$0"
set _clas12_root = "$cwd"

# When run directly, use the directory that contains this file.
if ("$_clas12_invocation:t" != "tcsh" && "$_clas12_invocation:t" != "csh" && "$_clas12_invocation" !~ "-*") then
    set _clas12_root = "$_clas12_invocation:h"
endif

# A user-set CLAS12_SAMPLES_DIR takes priority when sourcing from outside the checkout:
#   unset CLAS12_SAMPLES_DIR
#   unsetenv CLAS12_SAMPLES_DIR
#   setenv CLAS12_SAMPLES_DIR /shared/path/CLAS12-sample-generator
#   source "$CLAS12_SAMPLES_DIR/run.csh"
# The value remains until `unsetenv CLAS12_SAMPLES_DIR`.
if ($?CLAS12_SAMPLES_DIR) then
    set _clas12_root = "$CLAS12_SAMPLES_DIR"
endif

# Make the path absolute before any cleanup or Git command.
set _clas12_root = `cd "$_clas12_root" && pwd`

# Require both Git data and this project's workflow driver so cleanup cannot target another directory.
if (! -d "$_clas12_root/.git" || ! -f "$_clas12_root/src/launcher/workflow.py") then
    echo "Error: Cannot identify the CLAS12-sample-generator Git checkout: $_clas12_root"

    # Return through the shared block so a sourced shell stays open.
    set CLAS12_SAMPLE_STATUS = 1
    goto clas12_launcher_finish
endif

# Handle help and an empty command before updating the checkout.
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
    echo "    --config config/samples/genie-gst.conf --input 'GST_GLOB' --output OUTPUT_PARENT"
    echo ""
    echo "Submit completed LUND files:"
    echo "  source run.csh --workflow submit --lund-dir RUN/lundfiles"
    echo ""
    echo "Build without running a workflow payload:"
    echo "  source run.csh --workflow create-lund --source uniform --build true --run false"
    echo ""
    echo "Run 'source run.csh --help' for launcher options."
    echo "After selecting a create-lund source, add '-- --help' for its sample options."
    set CLAS12_SAMPLE_STATUS = 2
    goto clas12_launcher_finish
endif
# endregion

# Submission selection -------------------------------------------------------

# region Submission selection
# Detect submission so its arguments can be checked before the checkout update.
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
    # Help and invalid arguments must not update the server checkout.
    python3 "$_clas12_root/src/workflows/slurm-submission/resolve_inputs.py" --check-arguments $_clas12_submission_args:q
    set CLAS12_SAMPLE_STATUS = $status
    if ($CLAS12_SAMPLE_STATUS != 0) goto clas12_launcher_finish
    foreach _clas12_argument ($_clas12_submission_args:q)
        if ("$_clas12_argument" == "--help") goto clas12_launcher_finish
    end
endif
# endregion Submission selection

# Server mirror update --------------------------------------------------------

# region Server mirror update
# Enter the verified checkout and hide pushd's extra output.
pushd "$_clas12_root" > /dev/null

# Missing color or logo helpers do not stop the workflow.
if (-f src/launcher/presentation/set_colors.csh) source src/launcher/presentation/set_colors.csh
if (-f src/launcher/presentation/print_logo.csh) source src/launcher/presentation/print_logo.csh

# Tests may explicitly skip the server update. Normal ifarm use always updates.
set _clas12_skip_server_sync = 0
if ($?CLAS12_SKIP_SERVER_SYNC) then
    if ("$CLAS12_SKIP_SERVER_SYNC" == "1") set _clas12_skip_server_sync = 1
endif

# Run the updater in a child shell so it cannot close the shell that sourced run.csh.
# `src/launcher/checkout/code_updater.csh` performs, in order:
#   - `git rev-parse --show-toplevel` to require a recognized worktree;
#   - `git clean -fxd -e build/ -e build` to remove server-only untracked and ignored content while
#     retaining the reusable build tree;
#   - `git reset --hard` to discard server-side tracked edits;
#   - `git pull` to obtain the configured upstream revision;
#   - `git submodule sync --recursive` and `git submodule update --init --recursive` to check out
#     the recorded external revisions; and
#   - `git log -1 --oneline` plus `git branch --show-current` to report the resulting checkout.
# Any updater failure stops environment setup and the selected workflow.
if ($_clas12_skip_server_sync == 1) then
    echo "${SYSTEM_COLOR}Skipping ifarm checkout replacement (explicit local-development override).${RESET_COLOR}"
    set CLAS12_SAMPLE_STATUS = 0
else
    echo "${INFO_COLOR}Updating disposable ifarm checkout at:${RESET_COLOR}\n${_clas12_root}"
    tcsh -f src/launcher/checkout/code_updater.csh
    set CLAS12_SAMPLE_STATUS = $status
endif

# Reload colors because the update may have changed their names or values.
if ($CLAS12_SAMPLE_STATUS == 0) then
    if (-f src/launcher/presentation/set_colors.csh) then
        source src/launcher/presentation/set_colors.csh
        set CLAS12_SAMPLE_STATUS = $status
    else
        echo "Error: the synchronized checkout is missing src/launcher/presentation/set_colors.csh."
        set CLAS12_SAMPLE_STATUS = 1
    endif
endif

# Source the environment so its settings reach the Python driver and child programs.
if ($CLAS12_SAMPLE_STATUS == 0 && $_clas12_submit == 0 && -f src/launcher/environment/set_environment.csh) then
    source src/launcher/environment/set_environment.csh
    set CLAS12_SAMPLE_STATUS = $status
endif
# endregion

# Workflow dispatch -----------------------------------------------------------

# region Workflow dispatch
# Submission uses its shell bridge; LUND creation uses the Python workflow driver.
if ($CLAS12_SAMPLE_STATUS == 0) then
    if ($_clas12_submit == 1) then
        source src/workflows/slurm-submission/setup_and_submit.csh $_clas12_submission_args:q
        set CLAS12_SAMPLE_STATUS = $status
    else
        python3 src/launcher/workflow.py $argv:q
        set CLAS12_SAMPLE_STATUS = $status
    endif
endif

# Return to the directory from which the user sourced run.csh.
popd > /dev/null
# endregion

# Caller status ---------------------------------------------------------------

# region Caller status
# Every completion path returns through this block.
clas12_launcher_finish:

# Remove temporary variables but keep CLAS12_SAMPLE_STATUS for the user.
unset _clas12_invocation _clas12_root
if ($?_clas12_submit) unset _clas12_submit
if ($?_clas12_submission_args) unset _clas12_submission_args
if ($?_clas12_argument) unset _clas12_argument
if ($?_clas12_skip_server_sync) unset _clas12_skip_server_sync

# Use a child shell to return the result without closing the user's sourced shell.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion
