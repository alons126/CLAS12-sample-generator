#!/bin/csh

echo
echo

foreach TEMP_BEAM_E ( 2070MeV 4029MeV 5986MeV )

unset BEAM_E
setenv BEAM_E ${TEMP_BEAM_E}
# setenv BEAM_E 2070MeV
# setenv BEAM_E 4029MeV
# setenv BEAM_E 5986MeV
echo "BEAM_E: ${BEAM_E}"
echo

unset JOB_OUT_PATH
setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}
echo "JOB_OUT_PATH: ${JOB_OUT_PATH}"
echo

unset JOB_OUT_PATH_1E
setenv JOB_OUT_PATH_1E ${JOB_OUT_PATH}/OutPut_Tester_e_Tester_e
# setenv JOB_OUT_PATH_1E ${JOB_OUT_PATH}/OutPut_Tester_e_1e
echo "JOB_OUT_PATH_1E: ${JOB_OUT_PATH_1E}"
echo

unset SUBMIT_SCRIPT_PATH

if ("${BEAM_E}" == "2070MeV") then
    # echo "- Setting SUBMIT_SCRIPT_PATH for 2 GeV --------------------------------"
    setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_2GeV/
else if ("${BEAM_E}" == "4029MeV") then
    # echo "- Setting SUBMIT_SCRIPT_PATH for 4 GeV --------------------------------"
    setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_4GeV/
else if ("${BEAM_E}" == "5986MeV") then
    # echo "- Setting SUBMIT_SCRIPT_PATH for 6 GeV --------------------------------"
    setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_6GeV/
endif

echo "SUBMIT_SCRIPT_PATH: ${SUBMIT_SCRIPT_PATH}"
echo

module unload gemc
module load gemc/5.10
echo
echo "GEMC_DATA_DIR: ${GEMC_DATA_DIR}"
echo

# echo
# echo "Pulling updates..."
# git pull
# echo

# echo
# echo "Clearing farm_out directory..."
# rm /u/scifarm/farm_out/asportes/*
# echo

echo
echo "Removing old directory structure for MC simulation here..."
rm -rf ${JOB_OUT_PATH_1E}/mchipo
rm -rf ${JOB_OUT_PATH_1E}/reconhipo
rm -rf ${JOB_OUT_PATH_1E}/rootfiles
echo

echo
echo "Setting up directory structure for MC simulation here..."
mkdir ${JOB_OUT_PATH_1E}/mchipo ${JOB_OUT_PATH_1E}/reconhipo ${JOB_OUT_PATH_1E}/rootfiles
echo

echo
echo "Submitting Tester_e_1e sbatch job at ${BEAM_E}..."
sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_1e.sh
echo
end