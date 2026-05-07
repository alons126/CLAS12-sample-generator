#!/bin/csh

foreach TEMP_BEAM_E ( 2070MeV )
# foreach TEMP_BEAM_E ( 4029MeV )
# foreach TEMP_BEAM_E ( 2070MeV 4029MeV 5986MeV )

# Job parameters
# ============================================================================
unset BEAM_E
setenv BEAM_E ${TEMP_BEAM_E}

unset PRINT_OUT_COLOR

if ("${BEAM_E}" == "2070MeV") then
    setenv PRINT_OUT_COLOR '\033[31m'
else if ("${BEAM_E}" == "4029MeV") then
    setenv PRINT_OUT_COLOR '\033[32m'
else if ("${BEAM_E}" == "5986MeV") then
    setenv PRINT_OUT_COLOR '\033[33m'
endif

echo
echo "${PRINT_OUT_COLOR}- Job parameters ------------------------------------------------------\033[0m"
echo

echo "${PRINT_OUT_COLOR}BEAM_E:\033[0m ${BEAM_E}"
echo

unset CLEAR_FARM_OUT
setenv CLEAR_FARM_OUT 0 ## 1 for true
echo "${PRINT_OUT_COLOR}CLEAR_FARM_OUT:\033[0m ${CLEAR_FARM_OUT}"

unset CANCEL_PREVIOUS_JOBS
setenv CANCEL_PREVIOUS_JOBS 0 ## 1 for true
echo "${PRINT_OUT_COLOR}CANCEL_PREVIOUS_JOBS:\033[0m ${CANCEL_PREVIOUS_JOBS}"

unset USE_GEMC_5_10
setenv USE_GEMC_5_10 1 ## 1 for true
echo "${PRINT_OUT_COLOR}USE_GEMC_5_10:\033[0m ${USE_GEMC_5_10}"
echo

unset JOB_OUT_PATH
# setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}_ConstPn_lH2
# setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}_ConstPn
setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}_2
# setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}
echo "${PRINT_OUT_COLOR}JOB_OUT_PATH:\033[0m ${JOB_OUT_PATH}"

unset JOB_OUT_PATH_1E
setenv JOB_OUT_PATH_1E ${JOB_OUT_PATH}/OutPut_1e
echo "${PRINT_OUT_COLOR}JOB_OUT_PATH_1E:\033[0m ${JOB_OUT_PATH_1E}"

# unset JOB_OUT_PATH_EP
# setenv JOB_OUT_PATH_EP ${JOB_OUT_PATH}/OutPut_ep
# echo "${PRINT_OUT_COLOR}JOB_OUT_PATH_EP:\033[0m ${JOB_OUT_PATH_EP}"

# unset JOB_OUT_PATH_EN
# setenv JOB_OUT_PATH_EN ${JOB_OUT_PATH}/OutPut_en
# echo "${PRINT_OUT_COLOR}JOB_OUT_PATH_EN:\033[0m ${JOB_OUT_PATH_EN}"
echo

# Setting SUBMIT_SCRIPT_PATH for 2 GeV
# ============================================================================

unset RUNNING_DIR
setenv RUNNING_DIR `pwd`
echo "${PRINT_OUT_COLOR}RUNNING_DIR::\033[0m ${RUNNING_DIR}"
echo

if ("${BEAM_E}" == "2070MeV") then
    echo "${PRINT_OUT_COLOR}- Setting SUBMIT_SCRIPT_PATH for 2 GeV --------------------------------\033[0m"
    setenv SUBMIT_SCRIPT_PATH ${RUNNING_DIR}/Uniform_sample_2GeV/
    # setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_2GeV/
    echo
else if ("${BEAM_E}" == "4029MeV") then
    echo "${PRINT_OUT_COLOR}- Setting SUBMIT_SCRIPT_PATH for 4 GeV --------------------------------\033[0m"
    setenv SUBMIT_SCRIPT_PATH ${RUNNING_DIR}/Uniform_sample_4GeV/
    # setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_4GeV/
    echo
else if ("${BEAM_E}" == "5986MeV") then
    echo "${PRINT_OUT_COLOR}- Setting SUBMIT_SCRIPT_PATH for 6 GeV --------------------------------\033[0m"
    setenv SUBMIT_SCRIPT_PATH ${RUNNING_DIR}/Uniform_sample_6GeV/
    # setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_6GeV/
    echo
endif

echo "${PRINT_OUT_COLOR}SUBMIT_SCRIPT_PATH:\033[0m ${SUBMIT_SCRIPT_PATH}"
echo

# Re-pulling repository
# ============================================================================
echo "${PRINT_OUT_COLOR}- Re-pulling repository -----------------------------------------------\033[0m"
echo
echo "${PRINT_OUT_COLOR}Pulling updates...\033[0m"
git reset --hard
git pull
echo

# Clearing farm_out directory
# ============================================================================

# Optionally clear the farm_out directory
if ("${BEAM_E}" == "2070MeV") then
    if ("${CLEAR_FARM_OUT}" == "1") then
        echo
        echo "${PRINT_OUT_COLOR}- Clearing farm_out directory -----------------------------------------\033[0m"
        cd /u/scifarm/farm_out/asportes/
        rm *
        cd -
        echo
    endif
endif

# Canceling previous jobs
# ============================================================================

# Optionally cancel previous jobs
if ("${BEAM_E}" == "2070MeV") then
    if ("${CANCEL_PREVIOUS_JOBS}" == "1") then
        echo
        echo "${PRINT_OUT_COLOR}- Canceling previous jobs ---------------------------------------------\033[0m"
        scancel --user=asportes
        echo
    endif
endif

# Use GEMC 5.10
# ============================================================================

# Optionally use GEMC 5.10
if ("${USE_GEMC_5_10}" == "1") then
    echo
    echo "${PRINT_OUT_COLOR}- Reverting to GEMC 5.10 ----------------------------------------------\033[0m"
    module unload gemc
    module load gemc/5.10
    echo
endif
# if ("${BEAM_E}" == "2070MeV") then
#     if ("${USE_GEMC_5_10}" == "1") then
#         echo
#         echo "${PRINT_OUT_COLOR}- Reverting to GEMC 5.10 ----------------------------------------------\033[0m"
#         module unload gemc
#         module load gemc/5.10
#         echo
#     endif
# endif

echo "${PRINT_OUT_COLOR}GEMC_DATA_DIR:\033[0m ${GEMC_DATA_DIR}"
echo

# Removing old directory structure for MC simulation here
# ============================================================================

echo
echo "${PRINT_OUT_COLOR}- Removing old directory structure for MC simulation here -------------\033[0m"
rm -rf ${JOB_OUT_PATH_1E}/mchipo
rm -rf ${JOB_OUT_PATH_1E}/reconhipo
rm -rf ${JOB_OUT_PATH_1E}/rootfiles

# rm -rf ${JOB_OUT_PATH_EP}/mchipo
# rm -rf ${JOB_OUT_PATH_EP}/reconhipo
# rm -rf ${JOB_OUT_PATH_EP}/rootfiles

# rm -rf ${JOB_OUT_PATH_EN}/mchipo
# rm -rf ${JOB_OUT_PATH_EN}/reconhipo
# rm -rf ${JOB_OUT_PATH_EN}/rootfiles
echo

echo
echo "${PRINT_OUT_COLOR}- Setting up directory structure for MC simulation here ---------------\033[0m"
mkdir ${JOB_OUT_PATH_1E}/mchipo ${JOB_OUT_PATH_1E}/reconhipo ${JOB_OUT_PATH_1E}/rootfiles
# mkdir ${JOB_OUT_PATH_EP}/mchipo ${JOB_OUT_PATH_EP}/reconhipo ${JOB_OUT_PATH_EP}/rootfiles
# mkdir ${JOB_OUT_PATH_EN}/mchipo ${JOB_OUT_PATH_EN}/reconhipo ${JOB_OUT_PATH_EN}/rootfiles
echo

# Submitting jobs
# ============================================================================

echo
echo "${PRINT_OUT_COLOR}- Submitting jobs -----------------------------------------------------\033[0m"
echo

echo "${PRINT_OUT_COLOR}Submitting 1e sbatch job...\033[0m"
sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_1e.sh
echo

# echo "${PRINT_OUT_COLOR}Submitting ep sbatch job...\033[0m"
# sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_ep.sh
# echo

# echo "${PRINT_OUT_COLOR}Submitting en sbatch job...\033[0m"
# sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_en.sh
# echo

end