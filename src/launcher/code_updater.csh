#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# code_updater.csh -------------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Refresh the disposable ifarm checkout from its configured remote branch.
#
# Purpose:
#   Make the server checkout match the pushed project revision before a workflow starts.
#
# Workflow:
#   Load banner colors -> verify the Git checkout -> clean server-only files -> reset tracked files ->
#   pull the branch -> update submodules -> print the selected commit and branch.
#
# Inputs:
#   The current Git checkout, its configured upstream branch, and the build-directory exclusions.
#
# Outputs:
#   A refreshed checkout. Build directories are kept; other untracked files and tracked local changes
#   are removed under the disposable-server contract.
#
# Usage:
#   This helper is run by run.csh. Run it from the verified repository root.
#
# Failure:
#   A failed Git or submodule command returns a nonzero status and stops the update.

# Color and banner setup -------------------------------------------------------------------------------------------------------------------------------------------------

# region Color and banner setup

if ( -f ./src/launcher/environment/set_banners.csh ) then
    source ./src/launcher/environment/set_banners.csh
    # printf "${SYSTEM_COLOR}-->${RESET_COLOR} %b\n" "${COMPLETION_COLOR}Color environment loaded.${RESET_COLOR}"
    echo
else
    echo "\033[31mError:\033[0m the following file does not exist: ./src/launcher/environment/set_banners.csh\n"
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
if ( $status != 0 ) exit 1

git clean -fxd -e build/ -e build
if ( $status != 0 ) exit 1

echo ""
# endregion

# Remote synchronization ------------------------------------------------------------------------------------------------------------------------------------------------

# region Remote synchronization

git reset --hard
if ( $status != 0 ) exit 1

git pull

if ( $status != 0 ) then
    echo ""
    set banner_title = "git pull failed. Aborting update script."
    set banner_color = "$ERROR_COLOR"
    code_banner
    echo ""
    exit 1
endif

git submodule sync --recursive
if ( $status != 0 ) exit 1

git submodule update --init --recursive
if ( $status != 0 ) then
    echo ""
    set banner_title = "git submodule update failed. Aborting update script."
    set banner_color = "$ERROR_COLOR"
    code_banner
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
