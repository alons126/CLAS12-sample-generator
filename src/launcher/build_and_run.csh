#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# build_and_run.csh --------------------------------------------------------------------
# Description:
#   Older entry point for building and running the project.
# 
# Purpose:
#   Pass the request to workflow.py without updating Git by default.
# 
# Workflow:
#   1. Find the checkout.
#   2. Pass all arguments to the Python driver.
#   3. Return its status without closing the shell that sourced this file.
# 
# Usage (csh/tcsh, including files named .sh):
#   source src/launcher/build_and_run.csh --workflow create-lund --source uniform --output runs/example
#
# CLI options (forwarded unchanged to workflow.py):
#   --run-settings FILE          Read build JSON (default: config/run.json).
#   --workflow create-lund       Select the LUND workflow (required by this driver).
#   --source uniform|physical    Select LUND event source (required).
#   --build true|false           Configure and build (JSON default: true).
#   --run true|false             Run the LUND executable (JSON default: true).
#   --build-dir DIRECTORY        Select CMake binary tree (JSON default: build/release).
#   --build-type TYPE            Select CMake build type (default: Release).
#   --jobs N                     Set positive parallel build workers (JSON default: 4).
#   --help                       Print launcher help.
#   Other sample options pass through workflow.py. See the selected LUND program's --help for details.
# 
# Inputs:
#   $argv carries launcher/child options; CLAS12_SAMPLES_DIR overrides the root.
# 
# Outputs:
#   CLAS12_SAMPLE_STATUS and $status contain the driver result.
# 
# Notes:
#   Source from the repository root unless CLAS12_SAMPLES_DIR is set.
#   The Python driver inherits the already-loaded server software environment.

# Source from the checkout root, or set CLAS12_SAMPLES_DIR when working elsewhere.
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
# Return the saved status. Do not use exit because this file is normally sourced.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"

# endregion
