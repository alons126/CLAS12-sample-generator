
#!/bin/tcsh

# ------------------------------------------------------------------------------------------
# set_environment.csh
# ------------------------------------------------------------------------------------------
# Purpose
# -------
# This script initializes runtime environment variables used by the 2N Analyzer framework.
# It is intended to be sourced, not executed, so that the variables it defines remain in the
# caller's shell environment.
#
# Main responsibilities
# ---------------------
# 1. Define colored output for terminal messages.
# 2. Set DIR_2N_ANALYSIS_CODE to the project root directory.
# 3. Detect the machine hostname.
# 4. Determine whether the code is running on Jefferson Lab infrastructure (ifarm).
# 5. Export IFARM_RUN so downstream scripts can adapt their behaviour.
# ------------------------------------------------------------------------------------------

# Print header banner
# COLOR_START and COLOR_END may not yet exist, so plain output is used first.

echo "${COLOR_START}====================================================================================================${COLOR_END}"
echo "${COLOR_START}= Updating environment                                                                             =${COLOR_END}"
echo "${COLOR_START}====================================================================================================${COLOR_END}"
echo ""

# Reset color variables to ensure predictable behavior
unset COLOR_START
setenv COLOR_START '\033[35m'

unset COLOR_END
setenv COLOR_END '\033[0m'

# Section header

echo "${COLOR_START}- Updating environment -----------------------------------------------------------------------------${COLOR_END}"
echo ""

# ------------------------------------------------------------------------------------------
# Set project root directory
# ------------------------------------------------------------------------------------------

# Remove previous value if present
unset DIR_2N_ANALYSIS_CODE

# Set the variable to the current working directory
# Backticks execute the command and capture the output
setenv DIR_2N_ANALYSIS_CODE `pwd`

# Print value for verification
echo "${COLOR_START}DIR_2N_ANALYSIS_CODE:${COLOR_END} ${DIR_2N_ANALYSIS_CODE}"
echo ""

# ------------------------------------------------------------------------------------------
# Detect host machine
# ------------------------------------------------------------------------------------------

unset ANALYSIS_HOSTNAME

# hostname command returns the current machine name
setenv ANALYSIS_HOSTNAME `hostname`

# Print hostname for debugging / logging
echo "${COLOR_START}ANALYSIS_HOSTNAME:${COLOR_END} ${ANALYSIS_HOSTNAME}"
echo ""

# ------------------------------------------------------------------------------------------
# Define string used to detect JLab machines
# ------------------------------------------------------------------------------------------

unset JLAB_TESTER

# Any hostname containing this substring will be treated as a JLab machine
setenv JLAB_TESTER "jlab.org"

echo "${COLOR_START}JLAB_TESTER:${COLOR_END} ${JLAB_TESTER}"

# ------------------------------------------------------------------------------------------
# Determine whether we are running on ifarm
# ------------------------------------------------------------------------------------------

unset IFARM_RUN

# tcsh pattern matching: "=~" tests whether the left side matches a wildcard pattern
# Here we check if ANALYSIS_HOSTNAME contains "jlab.org"

if ( "$ANALYSIS_HOSTNAME" =~ *"$JLAB_TESTER"* ) then

    echo "${COLOR_START}The hostname contains '$JLAB_TESTER'. Running the commands for this case.${COLOR_END}"

    # Flag indicating execution on JLab infrastructure
    setenv IFARM_RUN 1

else

    echo "${COLOR_START}The hostname does not contain '$JLAB_TESTER'. Running the alternate commands.${COLOR_END}"

    # Local or non‑JLab machine
    setenv IFARM_RUN 0

endif

# Print final result

echo "${COLOR_START}IFARM_RUN:${COLOR_END} ${IFARM_RUN}"
echo ""
