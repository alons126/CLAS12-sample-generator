#
# Created by Alon Sportes on 14/09/2026.
#

#!/bin/tcsh

# -------------------------------------------------------------------------------------------------
# Interpreter declaration
# -------------------------------------------------------------------------------------------------
# The script is executed with the tcsh shell. This ensures that tcsh syntax such as
# `if (...) then`, `$status`, and `set` variables works correctly regardless of the
# user's default login shell.

# ------------------------------------------------------------------------------------------
# code_updater.sh
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
# These variables control colored output in the terminal.  The script only defines
# them if they were not already defined by the calling environment (for example
# when run.csh has already set them).
#
# `$?VARIABLE` is a tcsh test that returns true if the variable exists.

if ( ! $?COLOR_START ) then
    set COLOR_START = "\033[35m"
endif

if ( ! $?COLOR_END ) then
    set COLOR_END = "\033[0m"
endif

if ( ! $?COLOR_ERR ) then
    set COLOR_ERR = "\033[31m"
endif

# Print an empty line to visually separate this script's output from previous
# terminal output.

echo ""
# Print a visible banner showing that the update script has started.  The long
# separator lines make it easy to identify the beginning of the update stage
# inside long logs produced on remote machines such as ifarm.

echo "${COLOR_START}====================================================================================================${COLOR_END}"
echo "${COLOR_START}= Running update script                                                                            =${COLOR_END}"
echo "${COLOR_START}====================================================================================================${COLOR_END}"
echo ""

# -------------------------------------------------------------------------------------------------
# Clean working tree
# -------------------------------------------------------------------------------------------------
# Remove leftover temporary or build files that are not tracked by git.
# This ensures the repository is in a predictable state before pulling
# updates or running builds.

# Print a section header describing the current operation.

echo "${COLOR_START}- Cleaning excessive files -------------------------------------------------------------------------${COLOR_END}"
echo ""

# Remove all untracked files and directories in the repository.
# Flags:
#   -f  force removal
#   -x  also remove files listed in .gitignore
#   -d  remove directories
#
# The `-e build/` exclusion prevents the build directory from being removed.
# This allows reuse of the existing CMake build when the source code has not
# changed, which significantly speeds up repeated runs on the SSH machine.

git rev-parse --show-toplevel
if ( $status != 0 ) exit 1

git clean -fxd -e build/ -e build
if ( $status != 0 ) exit 1

echo ""

# -------------------------------------------------------------------------------------------------
# Synchronize repository with remote
# -------------------------------------------------------------------------------------------------
# Reset local changes and pull the latest commits from the remote repository.
# This guarantees the working copy matches the latest version of the dev branch
# before building or running the analysis.

# Discard any local modifications to tracked files.
# This ensures the repository exactly matches the last committed state before
# pulling updates.

git reset --hard
if ( $status != 0 ) exit 1

# Fetch new commits from the remote repository and update the local branch.

git pull
# `$status` holds the exit code of the previous command.
# A non‑zero value means `git pull` failed (for example due to network
# issues or merge conflicts).  In that case the script aborts immediately.

if ( $status != 0 ) then
    echo ""
    echo "${COLOR_ERR}====================================================================================================${COLOR_END}"
    echo "${COLOR_ERR}= git pull failed. Aborting update script.                                                         =${COLOR_END}"
    echo "${COLOR_ERR}====================================================================================================${COLOR_END}"
    echo ""
    exit 1
endif

# Synchronize local submodule URLs with the committed .gitmodules file, then check out every
# submodule at the exact gitlink revision recorded by the pulled superproject commit. This is
# required by fresh clones and also advances an existing ifarm submodule when its pin changes.
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

# Display the latest commit in the repository.
# This helps verify which exact revision of the analysis code is being used.

echo "${COLOR_START}HEAD:${COLOR_END}"
git log -1 --oneline

echo ""

# Display the currently checked out branch.  This is particularly useful
# when running on remote systems to confirm that the expected branch
# (usually `dev`) is active.

echo "${COLOR_START}Branch:${COLOR_END}"
git branch --show-current

echo ""

# -------------------------------------------------------------------------------------------------
# Reload environment
# -------------------------------------------------------------------------------------------------
# Reinitialize environment variables and helper commands that may depend
# on the current repository state.  These scripts typically set paths,
# analysis directories, and screen session helpers.

# Source the environment setup script which defines variables such as
# DIR_CLAS12_SAMPLE_GENERATOR_CODE, IFARM_RUN, and other runtime settings.

# run.csh sources the environment after this child process succeeds.
# # Source the screen helper script that defines aliases and functions for
# # launching analysis runs inside detached screen sessions.

# if ( -f ./src/launcher/screen/setup_screen_commands.csh ) then
#     source ./src/launcher/screen/setup_screen_commands.csh
# else
#     echo "${COLOR_ERR}Missing screen setup script: ./src/launcher/screen/setup_screen_commands.csh${COLOR_END}"
# endif

echo ""
