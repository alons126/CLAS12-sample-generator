#!/bin/tcsh

#
# Created by Alon Sportes on 19/09/2026.
#

# Submission shell bridge -----------------------------------------------------
# Description:
#     Connect the sourced launcher to the Python submission program.
#
# Purpose:
#     Keep the sourced run.csh interface and shared terminal colors.
#
# Usage:
#     source run.csh --workflow submit --lund-dir RUN/lundfiles [options].
#
# CLI options (forwarded unchanged to submit.py and parsed by resolve_inputs.py):
#     --lund-dir DIRECTORY          Completed RUN/lundfiles; repeat for multiple samples.
#     --config FILE                 Optional key = value submission settings.
#     --execute                     Submit and replace simulation output; default: preview.
#     --source uniform|physical     Source when no manifest supplies it.
#     --beam-energy GeV             Truth beam energy when no manifest supplies it.
#     --rgm-target ID               Truth target identity when no manifest supplies it.
#     --channel NAME                Uniform 1e, eh, electron-tester, or a legacy label.
#     --hadron NAME                 Proton, neutron, pip, or pim for eh.
#     --hadron-region FD|CD         Eh hadron detector region.
#     --event-generator NAME        Physical input adapter; default: genie-gst without a manifest.
#     --tune NAME                   Physical tune; default: unknown without a manifest.
#     --q2-cut NAME                 Physical input Q2 label; no cut is applied here.
#     --prefix NAME                 LUND filename prefix; required without a manifest.
#     --gemc-version VERSION        GEMC resources; fallback default: 5.14.
#     --gemc-target-variation NAME  Detector target variation.
#     --gcard FILE / --yaml FILE    Detector and reconstruction input overrides.
#     --torus SCALE                 Beam-dependent torus override.
#     --num-jobs N                  First N LUND files; default: all completed files.
#     --events-per-job N            Event limit; required without a manifest.
#     --job-name NAME               Metadata-derived Slurm job name override.
#     --clas12tags-dir DIRECTORY    Custom clas12Tags checkout as GEMC_DATA_DIR.
#     --clear-farm-out true|false   Clear direct farm logs with --execute; default: false.
#     --farm-out DIRECTORY          farm_out directory when clearing it.
#     --fc-status 0|1               Legacy physical filename/report label; default: 0.
#     --help                        Print submission help before synchronization.
#
# Workflow:
#     Load colors -> check settings -> load the selected GEMC module -> call sbatch -> read the accepted
#     job ID -> write the submission log -> print the shared final status.
#
# Inputs:
#     quoted CLI arguments and the ifarm module/reconstruction environment.
#
# Outputs:
#     Preview prints the checks and Slurm command. With --execute, Python replaces only mchipo and
#     reconhipo, submits the arrays, prints each job ID, and saves each ID in the submission log.
#     Changes made for sbatch stay inside Python and do not change the user's shell.
#
# Failure:
#     Return Python's exit status without closing the user's shell.

# region Submission
set CLAS12_SAMPLE_STATUS = 1
if (-f ./src/launcher/presentation/set_colors.csh) then
    source ./src/launcher/presentation/set_colors.csh
    if ($status == 0) then
        python3 src/workflows/slurm-submission/submit.py $argv:q
        set CLAS12_SAMPLE_STATUS = $status
    endif
else
    echo "Error: the following file does not exist: ./src/launcher/presentation/set_colors.csh"
endif
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion Submission
