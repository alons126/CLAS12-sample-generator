#
# Created by Alon Sportes on 14/09/2026.
#

#!/bin/tcsh

# build_and_run.csh --------------------------------------------------------------------
# Description:
#   Build/run compatibility entry point.
# Purpose:
#   Delegate to workflow.py with Git pulling disabled by default; forwarded options may override it.
# Workflow:
#   1. Resolve the checkout from invocation, cwd or CLAS12_SAMPLES_DIR.
#   2. Forward quoted arguments to the shared Python driver.
#   3. Return its status without exiting the sourced parent shell.
# Usage (csh/tcsh, including files named .sh):
#   source src/launcher/build_and_run.csh --workflow create-lund --source uniform --output runs/example
# Inputs:
#   $argv carries launcher/child options; CLAS12_SAMPLES_DIR overrides the root.
# Outputs:
#   CLAS12_SAMPLE_STATUS and immediate $status report the driver result.
# Notes:
#   Source from the repository root unless CLAS12_SAMPLES_DIR is set.
#   The Python driver inherits the already-loaded server software environment.

# CLAS12 sample workflow entry point. Source from the repository root, or set
# CLAS12_SAMPLES_DIR when sourcing from elsewhere. Execution works from any cwd.
# Checkout discovery -----------------------------------------------------------

# region Checkout discovery
set _clas12_invocation = "$0"
set _clas12_root = "$cwd"
if ("$_clas12_invocation:t" != "tcsh" && "$_clas12_invocation:t" != "csh" && "$_clas12_invocation" !~ "-*") then
    set _clas12_root = "$_clas12_invocation:h:h:h"
endif
if ($?CLAS12_SAMPLES_DIR) then
    set _clas12_root = "$CLAS12_SAMPLES_DIR"
endif
# endregion

# Driver invocation ------------------------------------------------------------

# region Driver invocation
if (-f "$_clas12_root/src/launcher/workflow.py") then
    python3 "$_clas12_root/src/launcher/workflow.py" $argv:q
    set CLAS12_SAMPLE_STATUS = $status
else
    echo "Cannot find src/launcher/workflow.py. Source from the checkout root or set CLAS12_SAMPLES_DIR."
    set CLAS12_SAMPLE_STATUS = 1
endif
# endregion

# Caller status ---------------------------------------------------------------

# region Caller status
unset _clas12_invocation _clas12_root
# Last command propagates status to both executed and sourced callers.
# In particular, do not use exit here: it would close a sourced SSH shell.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"

# endregion
