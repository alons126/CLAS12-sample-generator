#!/bin/tcsh
# CLAS12 sample workflow entry point. Source from the repository root, or set
# CLAS12_SAMPLES_DIR when sourcing from elsewhere. Execution works from any cwd.
set _clas12_invocation = "$0"
set _clas12_root = "$cwd"
if ("$_clas12_invocation:t" != "tcsh" && "$_clas12_invocation:t" != "csh" && "$_clas12_invocation" !~ "-*") then
    set _clas12_root = "$_clas12_invocation:h"
endif
if ($?CLAS12_SAMPLES_DIR) then
    set _clas12_root = "$CLAS12_SAMPLES_DIR"
endif
if (-f "$_clas12_root/scripts/workflow.py") then
    python3 "$_clas12_root/scripts/workflow.py" --update-only $argv:q
    set CLAS12_SAMPLE_STATUS = $status
else
    echo "Cannot find scripts/workflow.py. Source from the checkout root or set CLAS12_SAMPLES_DIR."
    set CLAS12_SAMPLE_STATUS = 1
endif
unset _clas12_invocation _clas12_root
# Last command propagates status to both executed and sourced callers.
# In particular, do not use exit here: it would close a sourced SSH shell.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
