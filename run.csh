#!/bin/tcsh

# run.csh --------------------------------------------------------------------
# Description:
#   Authoritative ifarm checkout entry point.
# Purpose:
#   Replace the disposable server clone with the remote revision, load its environment, then build
#   and run one configured CLAS12 sample workflow.
# Workflow:
#   1. Resolve and verify the checkout before running Git commands.
#   2. Run the intentionally destructive server-mirror updater in a child shell.
#   3. Load the updated environment and delegate build/test/run stages to workflow.py.
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
set _clas12_invocation = "$0"
set _clas12_root = "$cwd"
if ("$_clas12_invocation:t" != "tcsh" && "$_clas12_invocation:t" != "csh" && "$_clas12_invocation" !~ "-*") then
    set _clas12_root = "$_clas12_invocation:h"
endif
if ($?CLAS12_SAMPLES_DIR) then
    set _clas12_root = "$CLAS12_SAMPLES_DIR"
endif
set _clas12_root = `cd "$_clas12_root" && pwd`

if (! -d "$_clas12_root/.git" || ! -f "$_clas12_root/scripts/workflow.py") then
    echo "Cannot identify the CLAS12-sample-generator Git checkout: $_clas12_root"
    set CLAS12_SAMPLE_STATUS = 1
    unset _clas12_invocation _clas12_root
    /bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
endif
# endregion

# Server mirror update --------------------------------------------------------
# region Server mirror update
pushd "$_clas12_root" > /dev/null
if (-f scripts/environment/set_colors.csh) source scripts/environment/set_colors.csh
if (-f scripts/printers/print_logo.csh) source scripts/printers/print_logo.csh

if ($?CLAS12_SKIP_SERVER_SYNC && "$CLAS12_SKIP_SERVER_SYNC" == "1") then
    echo "Skipping ifarm checkout replacement (explicit local/test override)."
    set CLAS12_SAMPLE_STATUS = 0
else
    echo "Updating disposable ifarm checkout at $_clas12_root"
    tcsh -f scripts/code_updater.sh
    set CLAS12_SAMPLE_STATUS = $status
endif

if ($CLAS12_SAMPLE_STATUS == 0 && -f scripts/environment/set_environment.csh) then
    source scripts/environment/set_environment.csh
    set CLAS12_SAMPLE_STATUS = $status
endif
# endregion

# Workflow dispatch -----------------------------------------------------------
# region Workflow dispatch
if ($CLAS12_SAMPLE_STATUS == 0) then
    python3 scripts/workflow.py --git-pull false $argv:q
    set CLAS12_SAMPLE_STATUS = $status
endif
popd > /dev/null
# endregion

# Caller status ---------------------------------------------------------------
# region Caller status
unset _clas12_invocation _clas12_root
# Do not use exit: this file is normally sourced into the user's SSH shell.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion
