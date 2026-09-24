#!/bin/tcsh

#
# Created by Alon Sportes on 15/09/2026.
#

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
# 2. Set DIR_CLAS12_SAMPLE_GENERATOR_CODE to the project root directory.
# 3. Detect the machine hostname.
# 4. Determine whether the code is running on Jefferson Lab infrastructure (ifarm).
# 5. Export IFARM_RUN so downstream scripts can adapt their behaviour.
# ------------------------------------------------------------------------------------------

# Print header banner
# SYSTEM_COLOR and RESET_COLOR may not yet exist, so plain output is used first.

echo "${SYSTEM_COLOR}====================================================================================================${RESET_COLOR}"
echo "${SYSTEM_COLOR}= Updating environment                                                                             =${RESET_COLOR}"
echo "${SYSTEM_COLOR}====================================================================================================${RESET_COLOR}"
echo ""

source ./src/launcher/environment/set_colors.csh

# Section header

echo "${SYSTEM_COLOR}- Updating environment -----------------------------------------------------------------------------${RESET_COLOR}"
echo ""

# ------------------------------------------------------------------------------------------
# Set project root directory
# ------------------------------------------------------------------------------------------

# Remove both tcsh namespaces before exporting the authoritative value. A local variable created
# with `set` otherwise shadows a same-named environment variable created with `setenv`.
unset DIR_CLAS12_SAMPLE_GENERATOR_CODE
unsetenv DIR_CLAS12_SAMPLE_GENERATOR_CODE

# Set the variable to the current working directory
# Backticks execute the command and capture the output
setenv DIR_CLAS12_SAMPLE_GENERATOR_CODE `pwd`

# Print value for verification
echo "${SYSTEM_COLOR}DIR_CLAS12_SAMPLE_GENERATOR_CODE:${RESET_COLOR} ${DIR_CLAS12_SAMPLE_GENERATOR_CODE}"
echo ""

# ------------------------------------------------------------------------------------------
# Detect host machine
# ------------------------------------------------------------------------------------------

unset ANALYSIS_HOSTNAME
unsetenv ANALYSIS_HOSTNAME

# hostname command returns the current machine name
setenv ANALYSIS_HOSTNAME `hostname`

# Print hostname for debugging / logging
echo "${SYSTEM_COLOR}ANALYSIS_HOSTNAME:${RESET_COLOR} ${ANALYSIS_HOSTNAME}"
echo ""

# ------------------------------------------------------------------------------------------
# Define string used to detect JLab machines
# ------------------------------------------------------------------------------------------

unset JLAB_TESTER
unsetenv JLAB_TESTER

# Any hostname containing this substring will be treated as a JLab machine
setenv JLAB_TESTER "jlab.org"

echo "${SYSTEM_COLOR}JLAB_TESTER:${RESET_COLOR} ${JLAB_TESTER}"

# ------------------------------------------------------------------------------------------
# Determine whether we are running on ifarm
# ------------------------------------------------------------------------------------------

unset IFARM_RUN
unsetenv IFARM_RUN

# tcsh pattern matching: "=~" tests whether the left side matches a wildcard pattern
# Here we check if ANALYSIS_HOSTNAME contains "jlab.org"

if ( "$ANALYSIS_HOSTNAME" =~ *"$JLAB_TESTER"* ) then

    echo "${SYSTEM_COLOR}The hostname contains '$JLAB_TESTER'. Running the commands for this case.${RESET_COLOR}"

    # Flag indicating execution on JLab infrastructure
    setenv IFARM_RUN 1

else

    echo "${SYSTEM_COLOR}The hostname does not contain '$JLAB_TESTER'. Running the alternate commands.${RESET_COLOR}"

    # Local or non‑JLab machine
    setenv IFARM_RUN 0

endif

# Print final result

echo "${SYSTEM_COLOR}IFARM_RUN:${RESET_COLOR} ${IFARM_RUN}"
echo ""
