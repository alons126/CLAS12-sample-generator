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

if ( -f ./src/launcher/environment/set_colors.csh ) then
    source ./src/launcher/environment/set_colors.csh
    # printf "${COLOR_START}-->${COLOR_END} %b\n" "${COLOR_COMPLETION}Color environment loaded.${COLOR_END}"
    echo
else
    echo "\033[31mError:\033[0m the following file does not exist: ./src/launcher/environment/set_colors.csh\n"
    exit 1
endif

# Render every titled banner at 100 visible columns. Callers supply banner_title and banner_color;
# the helper calculates asymmetric padding when an odd number of spaces is required.
alias updater_banner 'set banner_title_length = `printf "%s" "$banner_title" | wc -c`; @ banner_padding = 96 - $banner_title_length; @ banner_padding_left = $banner_padding / 2; @ banner_padding_right = $banner_padding - $banner_padding_left; echo "${banner_color}////////////////////////////////////////////////////////////////////////////////////////////////////${COLOR_END}"; printf "${banner_color}//%*s%s%*s//${COLOR_END}\n" $banner_padding_left "" "${COLOR_END}$banner_title${banner_color}" $banner_padding_right ""; echo "${banner_color}////////////////////////////////////////////////////////////////////////////////////////////////////${COLOR_END}"; unset banner_title banner_color banner_title_length banner_padding banner_padding_left banner_padding_right'

set banner_title = "Running update script"
set banner_color = "$COLOR_START"
updater_banner
echo ""

# -------------------------------------------------------------------------------------------------
# Clean working tree
# -------------------------------------------------------------------------------------------------

echo "${COLOR_START}- Cleaning excessive files -------------------------------------------------------------------------${COLOR_END}"
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
    set banner_color = "$COLOR_ERR"
    updater_banner
    echo ""
    exit 1
endif

git submodule sync --recursive
if ( $status != 0 ) exit 1

git submodule update --init --recursive
if ( $status != 0 ) then
    echo ""
    set banner_title = "git submodule update failed. Aborting update script."
    set banner_color = "$COLOR_ERR"
    updater_banner
    echo ""
    exit 1
endif

echo ""

echo "${COLOR_START}HEAD:${COLOR_END}"
git log -1 --oneline

echo ""

echo "${COLOR_START}Branch:${COLOR_END}"
git branch --show-current

echo ""

# -------------------------------------------------------------------------------------------------
# Reload environment
# -------------------------------------------------------------------------------------------------

# run.csh sources the environment after this child process succeeds.

unalias updater_banner
echo ""
