#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# code_updater.csh -------------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Refresh the disposable ifarm checkout from its configured remote branch.
#
# Purpose:
#   Make the server checkout match the remote branch before a workflow starts.
#
# Workflow:
#   Load banner colors -> verify the Git checkout -> clean server-only files -> reset tracked files ->
#   pull the branch -> update submodules -> print the selected commit and branch.
#
# Inputs:
#   The current Git checkout, its remote branch, and the build directories that must be kept.
#
# Outputs:
#   A refreshed checkout. Build directories are kept; other untracked files and tracked local changes
#   are removed because the ifarm checkout is only a copy used for running jobs.
#
# Usage:
#   This helper is run by run.csh. Run it from the verified repository root.
#
# Failure:
#   A failed Git or submodule command returns a nonzero status and stops the update.

# Color and banner setup -------------------------------------------------------------------------------------------------------------------------------------------------

# region Color and banner setup

if ( -f ./src/launcher/presentation/set_banners.csh ) then
    source ./src/launcher/presentation/set_banners.csh
    echo
else
    echo "${ERROR_COLOR}Error:${RESET_COLOR} the following file does not exist: ./src/launcher/presentation/set_banners.csh\n"
    exit 1
endif

set banner_title = "Running update script"
set banner_color = "$SYSTEM_COLOR"
code_banner
echo ""
# endregion

# Checkout cleanup -------------------------------------------------------------------------------------------------------------------------------------------------------

# region Checkout cleanup

echo "${SYSTEM_COLOR}- Cleaning excessive files -------------------------------------------------------------------------${RESET_COLOR}"
echo ""

git rev-parse --show-toplevel
if ( $status != 0 ) then
    echo "${ERROR_COLOR}Error:${RESET_COLOR} Cannot identify the current Git worktree."
    exit 1
endif

git clean -fxd -e build/ -e build
if ( $status != 0 ) then
    echo "${ERROR_COLOR}Error:${RESET_COLOR} Git cleanup failed. Aborting update script."
    exit 1
endif

echo ""
# endregion

# Remote synchronization ------------------------------------------------------------------------------------------------------------------------------------------------

# region Remote synchronization

git reset --hard
if ( $status != 0 ) then
    echo "${ERROR_COLOR}Error:${RESET_COLOR} Git reset failed. Aborting update script."
    exit 1
endif

git pull

if ( $status != 0 ) then
    echo ""
    echo "${ERROR_COLOR}Error:${RESET_COLOR} git pull failed. Aborting update script."
    echo ""
    exit 1
endif

git submodule sync --recursive
if ( $status != 0 ) then
    echo "${ERROR_COLOR}Error:${RESET_COLOR} git submodule sync failed. Aborting update script."
    exit 1
endif

git submodule update --init --recursive
if ( $status != 0 ) then
    echo ""
    echo "${ERROR_COLOR}Error:${RESET_COLOR} git submodule update failed. Aborting update script."
    echo ""
    exit 1
endif

echo ""

echo "${SYSTEM_COLOR}HEAD:${RESET_COLOR}"
git log -1 --oneline

echo ""

echo "${SYSTEM_COLOR}Branch:${RESET_COLOR}"
git branch --show-current

echo ""
# endregion

# Return to launcher -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Return to launcher

# run.csh loads the environment after this script succeeds.

unalias code_banner
echo ""
# endregion
