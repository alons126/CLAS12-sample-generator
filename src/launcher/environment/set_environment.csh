#!/bin/tcsh

#
# Created by Alon Sportes on 15/09/2026.
#

# set_environment.csh ----------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   Set the launcher environment for the current checkout and host.
#
# Purpose:
#   Tell later scripts where the project is and whether they are running on Jefferson Lab ifarm.
#
# Workflow:
#   Load colors -> save the current checkout path -> read the hostname -> set IFARM_RUN from the host.
#
# Inputs:
#   The current working directory and hostname.
#
# Outputs:
#   DIR_CLAS12_SAMPLE_GENERATOR_CODE, ANALYSIS_HOSTNAME, JLAB_TESTER, and IFARM_RUN in the caller's shell.
#
# Usage:
#   Source this file from the repository root after checkout synchronization.
#
# Failure:
#   Failure to load the color file or query required host information returns a nonzero shell status.

# Shared colors ----------------------------------------------------------------------------------------------------------------------------------------------------------

# region Shared colors

# Load the synchronized palette before its first use. This is deliberately repeated even when
# run.csh already loaded colors: a Git update may have changed the palette contract while the parent
# shell was still executing the previously sourced launcher revision.
source ./src/launcher/environment/set_colors.csh
# endregion

# Environment banner -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Environment banner

echo "${SYSTEM_COLOR}====================================================================================================${RESET_COLOR}"
echo "${SYSTEM_COLOR}= Updating environment                                                                             =${RESET_COLOR}"
echo "${SYSTEM_COLOR}====================================================================================================${RESET_COLOR}"
echo ""
echo "${SYSTEM_COLOR}- Updating environment -----------------------------------------------------------------------------${RESET_COLOR}"
echo ""
# endregion

# Project root -----------------------------------------------------------------------------------------------------------------------------------------------------------

# region Project root

# Remove both tcsh namespaces before exporting the authoritative value. A local variable created
# with `set` otherwise shadows a same-named environment variable created with `setenv`.
unset DIR_CLAS12_SAMPLE_GENERATOR_CODE
unsetenv DIR_CLAS12_SAMPLE_GENERATOR_CODE

# Save the current directory as the project root.
setenv DIR_CLAS12_SAMPLE_GENERATOR_CODE `pwd`

# Show the resolved path.
echo "${SYSTEM_COLOR}DIR_CLAS12_SAMPLE_GENERATOR_CODE:${RESET_COLOR} ${DIR_CLAS12_SAMPLE_GENERATOR_CODE}"
echo ""
# endregion

# Host name --------------------------------------------------------------------------------------------------------------------------------------------------------------

# region Host name

unset ANALYSIS_HOSTNAME
unsetenv ANALYSIS_HOSTNAME

setenv ANALYSIS_HOSTNAME `hostname`

echo "${SYSTEM_COLOR}ANALYSIS_HOSTNAME:${RESET_COLOR} ${ANALYSIS_HOSTNAME}"
echo ""
# endregion

# Ifarm detection --------------------------------------------------------------------------------------------------------------------------------------------------------

# region Ifarm detection

unset JLAB_TESTER
unsetenv JLAB_TESTER

# A host containing this text is treated as a JLab machine.
setenv JLAB_TESTER "jlab.org"

echo "${SYSTEM_COLOR}JLAB_TESTER:${RESET_COLOR} ${JLAB_TESTER}"

unset IFARM_RUN
unsetenv IFARM_RUN

# `=~` checks whether the host contains the JLab text.
if ( "$ANALYSIS_HOSTNAME" =~ *"$JLAB_TESTER"* ) then

    echo "${SYSTEM_COLOR}The hostname contains '$JLAB_TESTER'. Running the commands for this case.${RESET_COLOR}"

    setenv IFARM_RUN 1

else

    echo "${SYSTEM_COLOR}The hostname does not contain '$JLAB_TESTER'. Running the alternate commands.${RESET_COLOR}"

    setenv IFARM_RUN 0

endif

echo "${SYSTEM_COLOR}IFARM_RUN:${RESET_COLOR} ${IFARM_RUN}"
echo ""
# endregion
