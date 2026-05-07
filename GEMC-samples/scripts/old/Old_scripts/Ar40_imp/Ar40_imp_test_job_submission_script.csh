#!/bin/csh

# Loop over fiducial cuts statuses
# foreach FC_STATUSES ( 1 )
foreach FC_STATUSES ( 0 )
# foreach FC_STATUSES ( 0 1 )

# Loop over target nuclei
# foreach SAMPLE_TARGET_NUCLEI ( H1 )
# foreach SAMPLE_TARGET_NUCLEI ( D2 )
foreach SAMPLE_TARGET_NUCLEI ( C12 )
# foreach SAMPLE_TARGET_NUCLEI ( Ar40 )
# foreach SAMPLE_TARGET_NUCLEI ( H1 D2 C12 Ar40 )

# Loop over GENIE tunes
foreach GENIE_TUNES ( G18_10a_00_000 )
# foreach GENIE_TUNES ( GEM21_11a_00_000 )
# foreach GENIE_TUNES ( G18_10a_00_000 GEM21_11a_00_000 )

# Loop over beam energies
foreach BEAM_ENERGIES ( 2070MeV )
# foreach BEAM_ENERGIES ( 4029MeV )
# foreach BEAM_ENERGIES ( 2070MeV 4029MeV )
# foreach BEAM_ENERGIES ( 2070MeV 4029MeV 5986MeV )

# Job parameters
# ============================================================================

# Set target nucleus
unsetenv SAMPLE_TARGET_NUCLEUS
setenv SAMPLE_TARGET_NUCLEUS ${SAMPLE_TARGET_NUCLEI}

# Set GENIE tune
unsetenv GENIE_TUNE
setenv GENIE_TUNE ${GENIE_TUNES}

# Set beam energy
unsetenv BEAM_E
setenv BEAM_E ${BEAM_ENERGIES}

# Set fiducial cuts status
setenv FC_STATUS_ENABLED ${FC_STATUSES}

unsetenv FC_STATUS
if ("${FC_STATUS_ENABLED}" == "1") then
    setenv FC_STATUS _wFC
else
    setenv FC_STATUS ""
endif

# Set printing color based on beam energy and fiducial cuts status
unsetenv PRINT_OUT_COLOR
if ("${BEAM_E}" == "2070MeV" && "${FC_STATUS_ENABLED}" == "0") then
    setenv PRINT_OUT_COLOR '\033[31m'
else if ("${BEAM_E}" == "2070MeV" && "${FC_STATUS_ENABLED}" == "1") then
    setenv PRINT_OUT_COLOR '\033[92m'
else if ("${BEAM_E}" == "4029MeV" && "${FC_STATUS_ENABLED}" == "0") then
    setenv PRINT_OUT_COLOR '\033[33m'
else if ("${BEAM_E}" == "4029MeV" && "${FC_STATUS_ENABLED}" == "1") then
    setenv PRINT_OUT_COLOR '\033[94m'
else if ("${BEAM_E}" == "5986MeV" && "${FC_STATUS_ENABLED}" == "0") then
    setenv PRINT_OUT_COLOR '\033[35m'
else if ("${BEAM_E}" == "5986MeV" && "${FC_STATUS_ENABLED}" == "1") then
    setenv PRINT_OUT_COLOR '\033[96m'
else
    echo "Unknown beam energy: ${BEAM_E}"
    exit 1
endif

# Set Q² cut based on energy
unsetenv Q2_CUT
if ("${BEAM_E}" == "2070MeV") then
    setenv Q2_CUT Q2_0_02
else if ("${BEAM_E}" == "4029MeV") then
    setenv Q2_CUT Q2_0_25
else if ("${BEAM_E}" == "5986MeV") then
    setenv Q2_CUT Q2_0_40
else
    echo "Unknown beam energy: ${BEAM_E}"
    exit 1
endif

# Set torus field based on beam energy
unsetenv TORUS_FIELD

# # For rgm_fall2021_C:
# setenv TORUS_FIELD 1

# For rgm_fall2021_C_v2_S or rgm_fall2021_C_v2_L:
if ("${BEAM_E}" == "2070MeV") then
    setenv TORUS_FIELD 0.5
else if ("${BEAM_E}" == "4029MeV" || "${BEAM_E}" == "5986MeV") then
    setenv TORUS_FIELD -1.0
else
    echo "Unknown torus field configuration: ${BEAM_E}"
    exit 1
endif

# Set GEMC data directory
unsetenv OUTPATH
# setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}
# setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}_Ar40_target_zpos_test
# setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}_Ar40_target_zpos_test_LATEST
setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_Ar_test
# setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_C_test
# setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_C_v2_S_test
# setenv OUTPATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_C_v2_L_test

echo
echo "${PRINT_OUT_COLOR}//////////////////////////////////////////////////////////////////////\033[0m"
echo "${PRINT_OUT_COLOR}// Setting GENIE slurm job submission                               //\033[0m"
echo "${PRINT_OUT_COLOR}//////////////////////////////////////////////////////////////////////\033[0m"
echo

echo "${PRINT_OUT_COLOR}- Sample parameters ---------------------------------------------------\033[0m"
echo ""

echo "${PRINT_OUT_COLOR}SAMPLE_TARGET_NUCLEUS:\033[0m ${SAMPLE_TARGET_NUCLEUS}"
echo ""

echo "${PRINT_OUT_COLOR}GENIE_TUNE:\033[0m ${GENIE_TUNE}"
echo ""

echo "${PRINT_OUT_COLOR}Q2_CUT:\033[0m ${Q2_CUT}"
echo ""

echo "${PRINT_OUT_COLOR}BEAM_E:\033[0m ${BEAM_E}"
echo ""

echo "${PRINT_OUT_COLOR}FC_STATUS:\033[0m ${FC_STATUS}"
echo ""

echo "${PRINT_OUT_COLOR}FC_STATUS_ENABLED:\033[0m ${FC_STATUS_ENABLED}"
echo ""

echo "${PRINT_OUT_COLOR}OUTPATH:\033[0m ${OUTPATH}"
echo ""

# Check if OUTPATH is a directory
if (! -d "${OUTPATH}") then
    echo "Error: Directory specified by OUTPATH does not exist: ${OUTPATH}"
    exit 1
endif

echo "${PRINT_OUT_COLOR}- Job parameters ------------------------------------------------------\033[0m"
echo ""

echo "${PRINT_OUT_COLOR}TORUS_FIELD:\033[0m ${TORUS_FIELD}"
echo ""

unsetenv CLEAR_FARM_OUT
setenv CLEAR_FARM_OUT 0 ## 1 for true
echo "${PRINT_OUT_COLOR}CLEAR_FARM_OUT:\033[0m ${CLEAR_FARM_OUT}"
echo ""

unsetenv CANCEL_PREVIOUS_JOBS
setenv CANCEL_PREVIOUS_JOBS 0 ## 1 for true
echo "${PRINT_OUT_COLOR}CANCEL_PREVIOUS_JOBS:\033[0m ${CANCEL_PREVIOUS_JOBS}"
echo ""

# Setting RUNNING_DIR
unsetenv RUNNING_DIR
setenv RUNNING_DIR `pwd`
echo "${PRINT_OUT_COLOR}RUNNING_DIR:\033[0m ${RUNNING_DIR}"
echo ""

# Check if RUNNING_DIR is a directory
if (! -d "${RUNNING_DIR}") then
    echo "Error: Directory specified by RUNNING_DIR does not exist: ${RUNNING_DIR}"
    exit 1
endif

# Setting gcard path
unsetenv GCARD_FILE_PATH
setenv GCARD_FILE_PATH /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12-config/gemc/dev
echo "${PRINT_OUT_COLOR}GCARD_FILE_PATH:\033[0m ${GCARD_FILE_PATH}"
echo ""

# Check if GCARD_FILE_PATH is a directory
if (! -d "${GCARD_FILE_PATH}") then
    echo "Error: Directory specified by GCARD_FILE_PATH does not exist: ${GCARD_FILE_PATH}"
    exit 1
endif

# Setting GCARD_FILE
unsetenv GCARD_FILE
if ("${SAMPLE_TARGET_NUCLEUS}" == "C12") then
    echo "${PRINT_OUT_COLOR}- Setting GCARD file for ${SAMPLE_TARGET_NUCLEUS} at ${BEAM_E} -------------------------------\033[0m"
    echo ""

    # # For rgm_fall2021_C:
    # setenv GCARD_FILE ${GCARD_FILE_PATH}/rgm_fall2021_C.gcard

    # For rgm_fall2021_C_v2_S or rgm_fall2021_C_v2_L:
    if ("${BEAM_E}" == "2070MeV") then
        setenv GCARD_FILE ${GCARD_FILE_PATH}/rgm_fall2021_C_v2_S_dev_test_${BEAM_E}.gcard
    else if ("${BEAM_E}" == "4029MeV") then
        setenv GCARD_FILE ${GCARD_FILE_PATH}/rgm_fall2021_C_v2_L_dev_test_${BEAM_E}.gcard
    else
        echo "Unknown gcard configuration for: ${SAMPLE_TARGET_NUCLEUS} at ${BEAM_E}"
        exit 1
    endif
else if ("${SAMPLE_TARGET_NUCLEUS}" == "Ar40") then
    echo "${PRINT_OUT_COLOR}- Setting GCARD file for ${SAMPLE_TARGET_NUCLEUS} at ${BEAM_E} ------------------------------\033[0m"
    echo ""

    setenv GCARD_FILE ${GCARD_FILE_PATH}/rgm_fall2021_Ar_dev_test_${BEAM_E}.gcard
else
    echo "Unknown gcard configuration for: ${SAMPLE_TARGET_NUCLEUS} at ${BEAM_E}"
    exit 1
endif

echo "${PRINT_OUT_COLOR}GCARD_FILE:\033[0m ${GCARD_FILE}"
echo

# Check if GCARD_FILE is a file
if (! -f "${GCARD_FILE}") then
    echo "Error: File specified by GCARD_FILE does not exist: ${GCARD_FILE}"
    exit 1
endif

# Setting yaml path
unsetenv YAML_FILE_PATH
echo "${PRINT_OUT_COLOR}- Setting YAML file for ${SAMPLE_TARGET_NUCLEUS} at ${BEAM_E} --------------------------------\033[0m"
if ("${BEAM_E}" == "2070MeV") then
    setenv YAML_FILE_PATH ${RUNNING_DIR}/Uniform_sample_2GeV
else if ("${BEAM_E}" == "4029MeV") then
    setenv YAML_FILE_PATH ${RUNNING_DIR}/Uniform_sample_4GeV
else if ("${BEAM_E}" == "5986MeV") then
    setenv YAML_FILE_PATH ${RUNNING_DIR}/Uniform_sample_6GeV
    echo ""
endif
echo ""

# Check if YAML_FILE_PATH is a directory
if (! -d "${YAML_FILE_PATH}") then
    echo "Error: Directory specified by YAML_FILE_PATH does not exist: ${YAML_FILE_PATH}"
    exit 1
endif

# Setting YAML_FILE
unsetenv YAML_FILE
if ("${BEAM_E}" == "2070MeV") then
    setenv YAML_FILE ${YAML_FILE_PATH}/rgm_fall2021-cv.yaml
else if ("${BEAM_E}" == "4029MeV") then
    setenv YAML_FILE ${YAML_FILE_PATH}/rgm_fall2021-ai_4Gev.yaml
else if ("${BEAM_E}" == "5986MeV") then
    setenv YAML_FILE ${YAML_FILE_PATH}/rgm_fall2021-ai_6Gev.yaml
endif
echo "${PRINT_OUT_COLOR}YAML_FILE:\033[0m ${YAML_FILE}"
echo

# Check if YAML_FILE is a file
if (! -f "${YAML_FILE}") then
    echo "Error: File specified by YAML_FILE does not exist: ${YAML_FILE}"
    exit 1
endif

# Re-pulling repository
echo "${PRINT_OUT_COLOR}- Re-pulling repository -----------------------------------------------\033[0m"
echo ""
echo "${PRINT_OUT_COLOR}Pulling updates...\033[0m"
git reset --hard # resets the current branch to the latest commit in the remote repository
git clean -fxd # removes untracked files and directories
echo ""
echo "${PRINT_OUT_COLOR}Pulling updates...\033[0m"
git pull
echo "HEAD:"
git log -1 --oneline # displays the latest commit in the current branch
echo ""

# Optionally clear the farm_out directory
if ("${CLEAR_FARM_OUT}" == "1") then
    echo "${PRINT_OUT_COLOR}- Clearing farm_out directory -----------------------------------------\033[0m"
    cd /u/scifarm/farm_out/asportes/
    rm -f *
    cd -
    echo
endif

# Optionally cancel previous jobs
if ("${CANCEL_PREVIOUS_JOBS}" == "1") then
    echo "${PRINT_OUT_COLOR}- Canceling previous jobs ---------------------------------------------\033[0m"
    scancel --user=asportes
    echo
endif

echo "${PRINT_OUT_COLOR}- Moving to GEMC dev --------------------------------------------------\033[0m"
module unload gemc
module load gemc/dev
echo

unsetenv GEMC_DATA_DIR
setenv GEMC_DATA_DIR /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12Tags
echo "${PRINT_OUT_COLOR}GEMC_DATA_DIR:\033[0m ${GEMC_DATA_DIR}"
echo

# Check if GEMC_DATA_DIR is a directory
if (! -d "${GEMC_DATA_DIR}") then
    echo "Error: File specified by GEMC_DATA_DIR does not exist: ${GEMC_DATA_DIR}"
    exit 1
endif

# Remove old output directories
echo "${PRINT_OUT_COLOR}- Removing old directory structure for MC simulation -------------------\033[0m"
rm -rf ${OUTPATH}/mchipo ${OUTPATH}/reconhipo
echo

# Create new output directories
echo "${PRINT_OUT_COLOR}- Setting up directory structure for MC simulation ---------------------\033[0m"
mkdir -p ${OUTPATH}/mchipo ${OUTPATH}/reconhipo
echo

if (! -d "${OUTPATH}/lundfiles" || "`ls -A ${OUTPATH}/lundfiles`" == "") then
    echo "Error: ${OUTPATH}/lundfiles must exist and must NOT be empty."
    exit 1
endif

# Check if output folders are valid
if (! -d "${OUTPATH}/mchipo" || "`ls -A ${OUTPATH}/mchipo`" != "") then
    echo "Error: ${OUTPATH}/mchipo must exist and be empty before proceeding."
    exit 1
endif
 
# Check if output folders are valid
if (! -d "${OUTPATH}/reconhipo" || "`ls -A ${OUTPATH}/reconhipo`" != "") then
    echo "Error: ${OUTPATH}/reconhipo must exist and be empty before proceeding."
    exit 1
endif
 
# Check if the directories were created successfully
echo "${PRINT_OUT_COLOR}Number of files in target directory (OUTPATH):\033[0m"
echo "${PRINT_OUT_COLOR}Number of lund files:     \t\033[0m `ls ${OUTPATH}/lundfiles | wc -l`"
echo "${PRINT_OUT_COLOR}Number of mchipo files:   \t\033[0m `ls ${OUTPATH}/mchipo | wc -l`"
echo "${PRINT_OUT_COLOR}Number of reconhipo files:\t\033[0m `ls ${OUTPATH}/reconhipo | wc -l`"
echo

# Submitting job
echo "${PRINT_OUT_COLOR}- Submitting jobs ------------------------------------------------------\033[0m"
echo ""
echo "${PRINT_OUT_COLOR}Submitting GENIE sbatch job...\033[0m"

unsetenv SLURM_JOB_NAME
# setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}
# setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}_Ar40_target_zpos_test
# setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}_Ar40_target_zpos_test_LATEST
setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_Ar_test
# setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_C_test
# setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_C_v2_S_test
# setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${BEAM_E}_${Q2_CUT}${FC_STATUS}_rgm_fall2021_C_v2_L_test
echo "${PRINT_OUT_COLOR}SLURM_JOB_NAME:\033[0m ${SLURM_JOB_NAME}"
echo ""

sbatch --job-name="${SLURM_JOB_NAME}" submit_GENIE_sample.sh || exit 1
echo ""

end  # End of loop over beam energies
end  # End of loop over GENIE tunes
end  # End of loop over target nuclei
end  # End of loop over fiducial cuts statuses
