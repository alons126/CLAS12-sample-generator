#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# -------------------------------------------------------------------------------------------------
# Interpreter declaration
# -------------------------------------------------------------------------------------------------
# The script is executed with the tcsh shell. This ensures that tcsh syntax such as
# `if (...) then`, `$status`, and `set` variables works correctly regardless of the
# user's default login shell.

# ------------------------------------------------------------------------------------------
# code_updater.csh
# ------------------------------------------------------------------------------------------
# Purpose
# -------
# Replaces the disposable ifarm checkout with the pushed CLAS12 sample-generator revision.
#
# Steps performed
# ---------------
# 1. Clean untracked build artifacts.
# 2. Reset local repository state.
# 3. Pull latest changes from remote.
# 4. Synchronize and initialize pinned Git submodules.
# 5. Display commit and branch information.
# 6. Return status to run.csh; environment loading happens there after the update.
# ------------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------------------
# Terminal color initialization
# -------------------------------------------------------------------------------------------------

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

# -------------------------------------------------------------------------------------------------
# Clean working tree
# -------------------------------------------------------------------------------------------------

echo "${SYSTEM_COLOR}- Cleaning excessive files -------------------------------------------------------------------------${RESET_COLOR}"
echo ""

git rev-parse --show-toplevel
if ( $status != 0 ) exit 1

git clean -fxd -e build/ -e build
if ( $status != 0 ) exit 1

echo ""

# -------------------------------------------------------------------------------------------------
# Synchronize repository with remote
# -------------------------------------------------------------------------------------------------

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

# -------------------------------------------------------------------------------------------------
# Reload environment
# -------------------------------------------------------------------------------------------------

# run.csh sources the environment after this child process succeeds.

unalias code_banner
echo ""
