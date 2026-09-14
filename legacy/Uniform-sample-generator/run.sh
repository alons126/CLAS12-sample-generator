#!/bin/tcsh

unsetenv CODE_PATH
setenv CODE_PATH `pwd`

# Check and set COLOR_START and COLOR_END if not already defined
if (! $?COLOR_START) then
    unset COLOR_START
    setenv COLOR_START '\033[35m'
endif

if (! $?COLOR_END) then
    unset COLOR_END
    setenv COLOR_END '\033[0m'
endif

echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo "${COLOR_START}= Running update script                                               =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

if ("$CODE_PATH" != "/Users/alon/Projects/Uniform-sample-generator") then
    echo "${COLOR_START}=======================================================================${COLOR_END}"
    echo "${COLOR_START}= Update review                                                       =${COLOR_END}"
    echo "${COLOR_START}=======================================================================${COLOR_END}"
    echo ""

    # Display the latest commit in the current branch. The -1 option limits the output to one commit, and the
    # --oneline option formats the output to show only the commit hash and the commit message in a single line.
    # This command is useful for quickly checking the latest commit in the current branch without displaying
    # the full commit history.

    echo "${COLOR_START}HEAD:${COLOR_END}"
    git log -1 --oneline
    echo ""

    echo "${COLOR_START}- Cleaning excessive files --------------------------------------------${COLOR_END}"
    echo ""

    # Clean the working tree by recursively removing files that are not under version control, starting from
    # the current directory. The -f option is used to force the removal of files, and the -d option is used
    # to remove untracked directories. The -x option is used to remove files that are ignored by git.

    # This command is useful for cleaning up the working tree and removing any untracked files or directories
    # that may have been created during development, like generated cut files and acceptance and weight files.

    git clean -fxd # removes untracked files and directories
    echo ""

    echo "${COLOR_START}- Pulling repository --------------------------------------------------${COLOR_END}"
    echo ""

    # This command is used to reset the current branch to the latest commit in the remote repository. The
    # --hard option is used to discard any local changes, and the git pull command is used to fetch and merge
    # the latest changes from the remote repository.

    git reset --hard
    git pull
    echo ""
endif

# Another display the latest commit in the current branch.
echo "${COLOR_START}HEAD:${COLOR_END}"
git log -1 --oneline
echo ""

echo "${COLOR_START}- Launching code -------------------------------------------------------${COLOR_END}"
echo ""

root ./CodeRun.cpp -l -q -b
echo ""

echo "${COLOR_START}- Operation finished --------------------------------------------------${COLOR_END}"
