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
    printf "${COLOR_START}-->${COLOR_END} %b\n" "${COLOR_COMPLETION}Color environment loaded.${COLOR_END}"
    echo
else
    printf "Error: the following file does not exist: ./src/launcher/environment/set_colors.csh\n"
    exit 1
endif


source ./src/launcher/environment/set_colors.csh

echo "CHECK 1:"
printenv COLOR_START

# ... next section of code ...

echo "CHECK 2:"
printenv COLOR_START

# ... next section ...

echo "CHECK 3:"
printenv COLOR_START

echo ""

printf '\033[33mLITERAL 33\033[0m\n'

printf "${COLOR_START}VARIABLE FORMAT${COLOR_END}\n"

printf '%bVARIABLE %%b%b\n' "$COLOR_START" "$COLOR_END"

printf '%s\n' "$COLOR_START" | od -An -tx1c

printf "${COLOR_START}====================================================================================================${COLOR_END}\n"
printf "${COLOR_START}= Running update script                                                                            =${COLOR_END}\n"
printf "${COLOR_START}====================================================================================================${COLOR_END}\n"
printf "\n"

# -------------------------------------------------------------------------------------------------
# Clean working tree
# -------------------------------------------------------------------------------------------------

printf "${COLOR_START}- Cleaning excessive files -------------------------------------------------------------------------${COLOR_END}\n"
printf "\n"

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
	printf "${COLOR_ERR}====================================================================================================${COLOR_END}\n"
	printf "${COLOR_ERR}= git pull failed. Aborting update script.                                                         =${COLOR_END}\n"
	printf "${COLOR_ERR}====================================================================================================${COLOR_END}\n"
    echo ""
    exit 1
endif

git submodule sync --recursive
if ( $status != 0 ) exit 1

git submodule update --init --recursive
if ( $status != 0 ) then
    echo ""
    echo "${COLOR_ERR}====================================================================================================${COLOR_END}"
    echo "${COLOR_ERR}= git submodule update failed. Aborting update script.                                             =${COLOR_END}"
    echo "${COLOR_ERR}====================================================================================================${COLOR_END}"
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

echo ""
