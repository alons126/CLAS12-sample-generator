#!/bin/tcsh

#
# Created by Alon Sportes on 19/09/2026.
#

# Submission shell bridge -----------------------------------------------------
# Purpose:
#     retain the sourced run.csh interface and shared terminal palette.
#
# Usage:
#     source run.csh --workflow submit --lund-dir RUN/lundfiles [options].
#
# Workflow:
#     initialize colors -> submit.py -> resolve_inputs.py -> sbatch -> protected payload.
#
# Inputs:
#     quoted CLI arguments and the preloaded ifarm GEMC/reconstruction environment.
#
# Outputs:
#     the same report and Slurm arrays; preview is default, --execute replaces only
#     mchipo/reconhipo and submits. Python owns resolved exports for its sbatch children;
#     it does not change the calling shell's sample variables or load software modules.
#
# Failure:
#     preserve Python's exit status without exiting the user's sourced shell.

# region Submission
set CLAS12_SAMPLE_STATUS = 1
if (-f ./src/launcher/environment/set_colors.csh) then
    source ./src/launcher/environment/set_colors.csh
    if ($status == 0) then
        python3 src/slurm-submission/submit.py $argv:q
        set CLAS12_SAMPLE_STATUS = $status
    endif
else
    echo "Error: the following file does not exist: ./src/launcher/environment/set_colors.csh"
endif
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion Submission
