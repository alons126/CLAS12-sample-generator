#!/bin/csh

unsetenv clear_farm_out§
setenv clear_farm_out false
echo "\033[35mclear_farm_out: \033[0m${clear_farm_out}"
echo

unset USE_GEMC_5_10
setenv USE_GEMC_5_10 1 ## 1 for true
echo "USE_GEMC_5_10: ${USE_GEMC_5_10}"
echo

unsetenv BEAM_E
setenv BEAM_E 2070MeV
# setenv BEAM_E 4029MeV
# setenv BEAM_E 5986MeV
echo "\033[35mBEAM_E: \033[0m${BEAM_E}"
echo

# Set paths based on BEAM_E
unsetenv JOB_OUT_PATH
# setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/${BEAM_E}
setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/${BEAM_E}_target_zpos_test
echo "\033[35mJOB_OUT_PATH: \033[0m${JOB_OUT_PATH}"
echo

# Check if JOB_OUT_PATH is a directory
if (! -d "${JOB_OUT_PATH}") then
    echo "\033[35mError: Directory specified by JOB_OUT_PATH does not exist:\033[0m ${JOB_OUT_PATH}"
    exit 1
endif

# unsetenv JOB_OUT_PATH_1E
# setenv JOB_OUT_PATH_1E ${JOB_OUT_PATH}/OutPut_1e
# echo "\033[35mJOB_OUT_PATH_1E: \033[0m${JOB_OUT_PATH_1E}"
# echo

# # Check if JOB_OUT_PATH_1E is a directory
# if (! -d "${JOB_OUT_PATH_1E}") then
#     echo "\033[35mError: Directory specified by JOB_OUT_PATH_1E does not exist:\033[0m ${JOB_OUT_PATH_1E}"
#     exit 1
# endif

# unsetenv JOB_OUT_PATH_EP
# setenv JOB_OUT_PATH_EP ${JOB_OUT_PATH}/OutPut_ep
# echo "\033[35mJOB_OUT_PATH_EP: \033[0m${JOB_OUT_PATH_EP}"
# echo

# # Check if JOB_OUT_PATH_EP is a directory
# if (! -d "${JOB_OUT_PATH_EP}") then
#     echo "\033[35mError: Directory specified by JOB_OUT_PATH_EP does not exist:\033[0m ${JOB_OUT_PATH_EP}"
#     exit 1
# endif

unsetenv JOB_OUT_PATH_EN
setenv JOB_OUT_PATH_EN ${JOB_OUT_PATH}/OutPut_en
echo "\033[35mJOB_OUT_PATH_EN: \033[0m${JOB_OUT_PATH_EN}"
echo

# Check if JOB_OUT_PATH_EN is a directory
if (! -d "${JOB_OUT_PATH_EN}") then
    echo "\033[35mError: Directory specified by JOB_OUT_PATH_EN does not exist:\033[0m ${JOB_OUT_PATH_EN}"
    exit 1
endif

# Determine the correct submit script path based on BEAM_E
unsetenv SUBMIT_SCRIPT_PATH
if ("${BEAM_E}" == "5986MeV") then
    setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_6GeV/
else if ("${BEAM_E}" == "4029MeV") then
    setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_4GeV/
else if ("${BEAM_E}" == "2070MeV") then
    setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_2GeV/
endif
echo "\033[35mSUBMIT_SCRIPT_PATH: \033[0m${SUBMIT_SCRIPT_PATH}"
echo

# Check if SUBMIT_SCRIPT_PATH is a directory
if (! -d "${SUBMIT_SCRIPT_PATH}") then
    echo "\033[35mError: Directory specified by SUBMIT_SCRIPT_PATH does not exist:\033[0m ${SUBMIT_SCRIPT_PATH}"
    exit 1
endif

echo "\033[35mPulling updates...\033[0m"
git pull
echo

# Optionally clear the farm_out directory
if ("${clear_farm_out}" == "true") then
    echo
    echo "\033[35mClearing farm_out directory...\033[0m"
    rm /u/scifarm/farm_out/asportes/*
    echo
endif

# Optionally use GEMC 5.10
if ("${USE_GEMC_5_10}" == "1") then
    echo
    echo "- Reverting to GEMC 5.10 ----------------------------------------------"
    module unload gemc
    module load gemc/5.10
    echo
endif

echo "\033[35mRemoving old directory structure for MC simulation here...\033[0m"
# rm -rf ${JOB_OUT_PATH_1E}/mchipo
# rm -rf ${JOB_OUT_PATH_1E}/reconhipo
# rm -rf ${JOB_OUT_PATH_1E}/rootfiles

# rm -rf ${JOB_OUT_PATH_EP}/mchipo
# rm -rf ${JOB_OUT_PATH_EP}/reconhipo
# rm -rf ${JOB_OUT_PATH_EP}/rootfiles

rm -rf ${JOB_OUT_PATH_EN}/mchipo
rm -rf ${JOB_OUT_PATH_EN}/reconhipo
rm -rf ${JOB_OUT_PATH_EN}/rootfiles
echo

echo "\033[35mSetting up directory structure for MC simulation here...\033[0m"
# mkdir ${JOB_OUT_PATH_1E}/mchipo ${JOB_OUT_PATH_1E}/reconhipo ${JOB_OUT_PATH_1E}/rootfiles
# mkdir ${JOB_OUT_PATH_EP}/mchipo ${JOB_OUT_PATH_EP}/reconhipo ${JOB_OUT_PATH_EP}/rootfiles
mkdir ${JOB_OUT_PATH_EN}/mchipo ${JOB_OUT_PATH_EN}/reconhipo ${JOB_OUT_PATH_EN}/rootfiles
echo

# echo
# echo "\033[35mSubmitting 1e sbatch job for BeamE = \033[0m${BEAM_E}\033[35m...\033[0m"
# sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_1e.sh
# echo

# echo "\033[35mSubmitting ep sbatch job for BeamE = \033[0m${BEAM_E}\033[35m...\033[0m"
# sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_ep.sh
# echo

echo "\033[35mSubmitting en sbatch job for BeamE = \033[0m${BEAM_E}\033[35m...\033[0m"
sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_en.sh
echo
