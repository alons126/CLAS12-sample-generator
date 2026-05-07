#!/bin/csh

echo "\033[1;35m//////////////////////////////////////////////////////////////////////\033[0m"
echo "\033[1;35m// Running update and submission script                             //\033[0m"
echo "\033[1;35m//////////////////////////////////////////////////////////////////////\033[0m"
echo

unset MY_SUBMISSION_DIR
setenv MY_SUBMISSION_DIR `pwd`
echo "\033[1;35mMY_SUBMISSION_DIR:\033[0m ${MY_SUBMISSION_DIR}"

alias pull_updates_script "source pull_updates_script.csh"

# Re-pulling repository
# ============================================================================
echo
echo "\033[1;35m- Re-pulling repository -----------------------------------------------\033[0m"
echo

# This command is used to reset the current branch to the latest commit in the remote repository. The
# --hard option is used to discard any local changes, and the git pull command is used to fetch and merge
# the latest changes from the remote repository.

git reset --hard
git pull
echo ""

# Display the latest commit in the current branch. The -1 option limits the output to one commit, and the
# --oneline option formats the output to show only the commit hash and the commit message in a single line.
# This command is useful for quickly checking the latest commit in the current branch without displaying
# the full commit history.

echo "HEAD:"
git log -1 --oneline
echo ""

# Clean the working tree by recursively removing files that are not under version control, starting from
# the current directory. The -f option is used to force the removal of files, and the -d option is used
# to remove untracked directories. The -x option is used to remove files that are ignored by git.

# This command is useful for cleaning up the working tree and removing any untracked files or directories
# that may have been created during development, like generated cut files and acceptance and weight files.

git clean -fxd # removes untracked files and directories
echo ""

echo "\033[1;35mPulling updates...\033[0m"
git pull
echo

# Sourcing submission scripts
# ============================================================================
echo "\033[1;35m- Sourcing submission scripts -----------------------------------------\033[0m"
echo
echo
source submission_script.csh
echo

echo
cd $MY_SUBMISSION_DIR
echo "\033[1;35m- FINISHED ------------------------------------------------------------\033[0m"
echo