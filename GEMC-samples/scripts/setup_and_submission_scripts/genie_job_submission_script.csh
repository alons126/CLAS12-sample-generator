#!/bin/csh

# A script to setup environment variables and paths for GENIE sample generation and submission, and then submit the jobs to the cluster using sbatch. This script is designed to be flexible and configurable through environment variables, allowing for easy adjustments to the simulation parameters, output paths, and other settings without needing to modify the core logic of the script. The script also includes checks to ensure that the necessary directories and files exist before attempting to submit the jobs, providing error messages if any required resources are missing. The GENIE sample generation and submission is performed for different beam energies, target nuclei, GENIE tunes, and fiducial cuts statuses, with the appropriate configurations and paths set for each case.

# Setup environment
# ======================================================================================================
source ./scripts/set_env.csh
echo

# Script banner
# ======================================================================================================
echo ""
echo "${SYSTEM_COLOR}///////////////////////////////////////////////////////////////////////${COLOR_END}"
printf "%s%s%s\n" "${SYSTEM_COLOR}//${COLOR_END}        Setting and submitting GENIE sample generation jobs        ${SYSTEM_COLOR}//${COLOR_END}"
echo "${SYSTEM_COLOR}///////////////////////////////////////////////////////////////////////${COLOR_END}"
echo ""

# Setup environment variables and paths for uniform sample generation and submission
# ======================================================================================================
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s\n" "${COLOR_START}= Setup environment variables and paths                               =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

unsetenv CLEAR_FAR_OUT
setenv CLEAR_FAR_OUT false
echo "${COLOR_START}CLEAR_FAR_OUT:${COLOR_END}       ${CLEAR_FAR_OUT}"
echo

unset CUSTOM_GEMC_VERSION
setenv CUSTOM_GEMC_VERSION true
echo "${COLOR_START}CUSTOM_GEMC_VERSION:${COLOR_END} ${CUSTOM_GEMC_VERSION}"
echo

unset GEMC_VERSION
setenv GEMC_VERSION 5.14
# setenv GEMC_VERSION dev
echo "${COLOR_START}GEMC_VERSION:${COLOR_END}        ${GEMC_VERSION}"
echo

unset NUM_OF_JOBS
# setenv NUM_OF_JOBS 1
setenv NUM_OF_JOBS 10
# setenv NUM_OF_JOBS 100
# setenv NUM_OF_JOBS 2500
# setenv NUM_OF_JOBS 5000
# setenv NUM_OF_JOBS 7500
# setenv NUM_OF_JOBS 10000
echo "${COLOR_START}NUM_OF_JOBS:${COLOR_END}         ${NUM_OF_JOBS}"
echo

# Main Script
# ======================================================================================================
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s%s%s\n"  "${COLOR_START}= " "Starting GENIE sample generation and submission     ${COLOR_START}=${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

# Set paths based on TEMP_BEAM_E and TARGET_VARIATION for uniform sample generation and submission. These environment variables will be used in the uniform sample generation and submission scripts to ensure that the correct paths and configurations are used for each beam energy and target variation.
unsetenv OUTPATH_BASE
setenv OUTPATH_BASE /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples
echo "${COLOR_START}OUTPATH_BASE: ${COLOR_END}${OUTPATH_BASE}"

# Check if OUTPATH_BASE is a directory
echo "${COLOR_START}--> Checking if ${COLOR_END}OUTPATH_BASE${COLOR_START} is a directory...${COLOR_END}"
if ( ! -d "${OUTPATH_BASE}" ) then
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${OUTPATH_BASE}"
    exit 1
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}OUTPATH_BASE exists.${COLOR_END}"
    echo
endif

unsetenv CLAS12TAGS_DIR
setenv CLAS12TAGS_DIR /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/alons126-clas12Tags
echo "${COLOR_START}CLAS12TAGS_DIR:${COLOR_END} ${CLAS12TAGS_DIR}"

# Check if CLAS12TAGS_DIR is a directory
echo "${COLOR_START}--> Checking if ${COLOR_END}CLAS12TAGS_DIR${COLOR_START} is a directory...${COLOR_END}"
if ( ! -d "${CLAS12TAGS_DIR}" ) then
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${CLAS12TAGS_DIR}"
    exit 1
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}CLAS12TAGS_DIR exists.${COLOR_END}"
    echo
endif

# Handle farm_out directory clearing
# ------------------------------------------------------------------------------------------------------
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s%s%s\n" "${COLOR_START}= Handling farm_out directory clearing and custom GEMC version        =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

# Optionally clear the farm_out directory
if ("${CLEAR_FAR_OUT}" == "true") then
    echo "${COLOR_START}Clearing farm_out directory...${COLOR_END}"
    echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
    rm /u/scifarm/farm_out/asportes/*
    echo
else
    echo "CLEAR_FAR_OUT$ ${COLOR_START}is set to '${COLOR_END}false${COLOR_START}', skipping farm_out directory clearing...${COLOR_END}"
    echo
endif

# Handle custom GEMC version loading based on environment variables
# ------------------------------------------------------------------------------------------------------
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s%s%s\n" "${COLOR_START}= Handling custom GEMC version                                        =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

# Optionally use custom GEMC version and set GEMC_DATA_DIR to a custom path
if ("${CUSTOM_GEMC_VERSION}" == "true") then
    echo "${COLOR_START}Loading GEMC version ${COLOR_END}${GEMC_VERSION}${COLOR_START}...${COLOR_END}"
    echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
    module unload gemc
    module load gemc/${GEMC_VERSION}
    echo

    # Set GEMC data directory to a custom path. This is important to ensure that the correct geometry and configuration files are used for the simulations, especially if using a custom or development version of GEMC.
    unsetenv GEMC_DATA_DIR
    setenv GEMC_DATA_DIR ${CLAS12TAGS_DIR}
    echo "${COLOR_START}GEMC_DATA_DIR:${COLOR_END} ${GEMC_DATA_DIR}"

    # Check if GEMC_DATA_DIR is a directory
    echo "${COLOR_START}--> Checking if ${COLOR_END}GEMC_DATA_DIR${COLOR_START} is a directory...${COLOR_END}"
    if ( ! -d "${GEMC_DATA_DIR}" ) then
        printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${GEMC_DATA_DIR}"
        exit 1
    else
        printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}GEMC_DATA_DIR exists.${COLOR_END}"
        echo
    endif
else
    echo "CUSTOM_GEMC_VERSION$ ${COLOR_START}is set to '${COLOR_END}false${COLOR_START}', skipping custom GEMC version loading...${COLOR_END}"
    echo
endif

# Loop over particle types
# ------------------------------------------------------------------------------------------------------
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s%s%s\n" "${COLOR_START}= Looping over samples                                         =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""
# Loop over fiducial cuts statuses
# ------------------------------------------------------------------------------------------------------
# foreach FC_STATUSES ( 1 )
foreach FC_STATUSES ( 0 )
# foreach FC_STATUSES ( 0 1 )

    # Loop over target nuclei
    # --------------------------------------------------------------------------------------------------
    foreach SAMPLE_TARGET_NUCLEI ( C12 )
    # foreach SAMPLE_TARGET_NUCLEI ( C12 Ar40 )
    # foreach SAMPLE_TARGET_NUCLEI ( H1 D2 C12 Ar40 )

        # Loop over GENIE tunes
        # ----------------------------------------------------------------------------------------------
        foreach GENIE_TUNES ( G18_10a_00_000 )
        # foreach GENIE_TUNES ( GEM21_11a_00_000 )
        # foreach GENIE_TUNES ( G18_10a_00_000 GEM21_11a_00_000 )

            # Loop over beam energies
            # ------------------------------------------------------------------------------------------
            foreach BEAM_E ( 2070MeV )
            # foreach BEAM_E ( 4029MeV )
            # foreach BEAM_E ( 5986MeV )
            # foreach BEAM_E ( 2070MeV 4029MeV )
            # foreach BEAM_E ( 2070MeV 4029MeV 5986MeV )
                echo
                echo "${COLOR_START}Processing GENIE sample for ${COLOR_END}${SAMPLE_TARGET_NUCLEI}${COLOR_START} (${COLOR_END}${GENIE_TUNES}${COLOR_START}) at beam energy ${COLOR_END}${BEAM_E}"
                echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
                echo

                # Job parameters
                # ============================================================================

                # Set target nucleus
                unsetenv SAMPLE_TARGET_NUCLEUS
                setenv SAMPLE_TARGET_NUCLEUS ${SAMPLE_TARGET_NUCLEI}

                # Set GENIE tune
                unsetenv GENIE_TUNE
                setenv GENIE_TUNE ${GENIE_TUNES}

                # Set beam energy
                unsetenv TEMP_BEAM_E
                setenv TEMP_BEAM_E ${BEAM_E}

                # Set fiducial cuts status
                setenv FC_STATUS_ENABLED ${FC_STATUSES}

                unsetenv FC_STATUS
                if ("${FC_STATUS_ENABLED}" == "1") then
                    setenv FC_STATUS _wFC
                else
                    setenv FC_STATUS ""
                endif

                # Set printing color based on beam energy and fiducial cuts status
                unsetenv TARGET_VARIATION
                if ("${TEMP_BEAM_E}" == "2070MeV" && "${SAMPLE_TARGET_NUCLEUS}" == "C12") then
                    setenv TARGET_VARIATION rgm_fall2021_C_S
                else if ("${TEMP_BEAM_E}" == "4029MeV" && "${SAMPLE_TARGET_NUCLEUS}" == "C12") then
                    setenv TARGET_VARIATION rgm_fall2021_C_L
                else if ("${TEMP_BEAM_E}" == "5986MeV" && "${SAMPLE_TARGET_NUCLEUS}" == "C12") then
                    setenv TARGET_VARIATION rgm_fall2021_Cx4
                else if ("${SAMPLE_TARGET_NUCLEUS}" == "Ar40") then
                    setenv TARGET_VARIATION rgm_fall2021_Ar
                # else
                #     setenv TARGET_VARIATION rgm_fall2021_Ar
                endif
                echo "${COLOR_START}TARGET_VARIATION:${COLOR_END} ${TARGET_VARIATION}"
                echo

                # Set printing color based on beam energy and fiducial cuts status
                unsetenv PRINT_OUT_COLOR
                if ("${TEMP_BEAM_E}" == "2070MeV" && "${FC_STATUS_ENABLED}" == "0") then
                    setenv PRINT_OUT_COLOR '\033[31m'
                else if ("${TEMP_BEAM_E}" == "2070MeV" && "${FC_STATUS_ENABLED}" == "1") then
                    setenv PRINT_OUT_COLOR '\033[92m'
                else if ("${TEMP_BEAM_E}" == "4029MeV" && "${FC_STATUS_ENABLED}" == "0") then
                    setenv PRINT_OUT_COLOR '\033[33m'
                else if ("${TEMP_BEAM_E}" == "4029MeV" && "${FC_STATUS_ENABLED}" == "1") then
                    setenv PRINT_OUT_COLOR '\033[94m'
                else if ("${TEMP_BEAM_E}" == "5986MeV" && "${FC_STATUS_ENABLED}" == "0") then
                    setenv PRINT_OUT_COLOR '\033[35m'
                else if ("${TEMP_BEAM_E}" == "5986MeV" && "${FC_STATUS_ENABLED}" == "1") then
                    setenv PRINT_OUT_COLOR '\033[96m'
                else
                    echo "Unknown beam energy: ${TEMP_BEAM_E}"
                    exit 1
                endif

                # Set Q² cut based on energy
                unsetenv Q2_CUT
                if ("${TEMP_BEAM_E}" == "2070MeV") then
                    setenv Q2_CUT Q2_0_02
                else if ("${TEMP_BEAM_E}" == "4029MeV") then
                    setenv Q2_CUT Q2_0_25
                else if ("${TEMP_BEAM_E}" == "5986MeV") then
                    setenv Q2_CUT Q2_0_40
                else
                    echo "Unknown beam energy: ${TEMP_BEAM_E}"
                    exit 1
                endif

                # Determine the correct submit script path based on TEMP_BEAM_E
                unsetenv TEMP_BEAM_E_ROUNDED
                if ("${TEMP_BEAM_E}" == "2070MeV") then
                    setenv TEMP_BEAM_E_ROUNDED 2GeV
                else if ("${TEMP_BEAM_E}" == "4029MeV") then
                    setenv TEMP_BEAM_E_ROUNDED 4GeV
                else if ("${TEMP_BEAM_E}" == "5986MeV") then
                    setenv TEMP_BEAM_E_ROUNDED 6GeV
                endif
                echo "${PRINT_OUT_COLOR}TEMP_BEAM_E_ROUNDED:${COLOR_END} ${TEMP_BEAM_E_ROUNDED}"
                echo

                # Set torus field based on beam energy
                unsetenv TORUS_FIELD
                if ("${TEMP_BEAM_E}" == "2070MeV") then
                    setenv TORUS_FIELD 0.5
                else if ("${TEMP_BEAM_E}" == "4029MeV" || "${TEMP_BEAM_E}" == "5986MeV") then
                    setenv TORUS_FIELD -1.0
                else
                    echo "Unknown torus field configuration: ${TEMP_BEAM_E}"
                    exit 1
                endif

                # Set OUTPATH directory
                unsetenv OUTPATH
                setenv OUTPATH ${OUTPATH_BASE}/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${TEMP_BEAM_E}_${Q2_CUT}${FC_STATUS}_GEMC5.14
                # setenv OUTPATH ${OUTPATH_BASE}/${SAMPLE_TARGET_NUCLEUS}/${GENIE_TUNE}/${TEMP_BEAM_E}_${Q2_CUT}${FC_STATUS}_devGEMC_${TARGET_VARIATION}

                echo "${PRINT_OUT_COLOR}//////////////////////////////////////////////////////////////////////${COLOR_END}"
                echo "${PRINT_OUT_COLOR}// Setting GENIE slurm job submission                               //${COLOR_END}"
                echo "${PRINT_OUT_COLOR}//////////////////////////////////////////////////////////////////////${COLOR_END}"
                echo

                echo "${PRINT_OUT_COLOR}- Sample parameters ---------------------------------------------------${COLOR_END}"
                echo ""

                echo "${PRINT_OUT_COLOR}SAMPLE_TARGET_NUCLEUS:${COLOR_END} ${SAMPLE_TARGET_NUCLEUS}"
                echo ""

                echo "${PRINT_OUT_COLOR}GENIE_TUNE:${COLOR_END} ${GENIE_TUNE}"
                echo ""

                echo "${PRINT_OUT_COLOR}Q2_CUT:${COLOR_END} ${Q2_CUT}"
                echo ""

                echo "${PRINT_OUT_COLOR}TEMP_BEAM_E:${COLOR_END} ${TEMP_BEAM_E}"
                echo ""

                echo "${PRINT_OUT_COLOR}FC_STATUS:${COLOR_END} ${FC_STATUS}"
                echo ""

                echo "${PRINT_OUT_COLOR}FC_STATUS_ENABLED:${COLOR_END} ${FC_STATUS_ENABLED}"
                echo ""

                echo "${PRINT_OUT_COLOR}OUTPATH:${COLOR_END} ${OUTPATH}"
                echo ""

                # Check if OUTPATH is a directory
                echo "${COLOR_START}--> Checking if ${COLOR_END}OUTPATH${COLOR_START} is a directory...${COLOR_END}"
                if ( ! -d "${OUTPATH}" ) then
                    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_WARNING_START}Warning:${COLOR_END}" " the following directory does not exist: ${OUTPATH}"
                    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_WARNING_START}Creating OUTPATH.${COLOR_END}"
                    mkdir ${OUTPATH}

                    # Check if OUTPATH is a directory
                    echo "${COLOR_START}----> Checking if ${COLOR_END}OUTPATH${COLOR_START} is a directory...${COLOR_END}"
                    if ( ! -d "${OUTPATH}" ) then
                        printf "${COLOR_START}---->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${OUTPATH}"
                        exit 1
                    else
                        printf "${COLOR_START}---->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}OUTPATH was created successfully.${COLOR_END}"
                        echo
                    endif
                else
                    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}OUTPATH exists.${COLOR_END}"
                    echo
                endif

                echo "${PRINT_OUT_COLOR}- Job parameters ------------------------------------------------------${COLOR_END}"
                echo ""

                echo "${PRINT_OUT_COLOR}TORUS_FIELD:${COLOR_END} ${TORUS_FIELD}"
                echo ""

                # Check if RUNNING_DIR is a directory
                echo "${COLOR_START}--> Checking if ${COLOR_END}RUNNING_DIR${COLOR_START} is a directory...${COLOR_END}"
                if ( ! -d "${RUNNING_DIR}" ) then
                    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${RUNNING_DIR}"
                    exit 1
                else
                    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}RUNNING_DIR exists.${COLOR_END}"
                    echo
                endif

                # Determine the correct submit script path based on TEMP_BEAM_E
                unsetenv REQUIREMENTS_PATH
                setenv REQUIREMENTS_PATH ${RUNNING_DIR}/Generation_files_${TEMP_BEAM_E_ROUNDED}
                echo "${PRINT_OUT_COLOR}REQUIREMENTS_PATH:${COLOR_END} ${REQUIREMENTS_PATH}"
                echo

                # Check if REQUIREMENTS_PATH is a directory
                echo "${PRINT_OUT_COLOR}--> Checking if ${COLOR_END}REQUIREMENTS_PATH${PRINT_OUT_COLOR} is a directory...${COLOR_END}"
                if ( ! -d "${REQUIREMENTS_PATH}" ) then
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${REQUIREMENTS_PATH}"
                    exit 1
                else
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}REQUIREMENTS_PATH exists.${COLOR_END}"
                    echo
                endif

                # Set GCARD_FILE and YAML_FILE paths based on TEMP_BEAM_E and TARGET_VARIATION. These will be used in the uniform sample generation and submission scripts to ensure that the correct configurations are used for each beam energy and target variation.
                # --------------------------------------------------------------------------------------------------
                echo "${PRINT_OUT_COLOR}Setting GCARD_FILE and YAML_FILE files based on TEMP_BEAM_E and TARGET_VARIATION${COLOR_END}"
                echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
                echo

                # Setting GCARD_FILE
                unsetenv GCARD_FILE
                setenv GCARD_FILE ${REQUIREMENTS_PATH}/${TARGET_VARIATION}_${TEMP_BEAM_E_ROUNDED}.gcard
                echo "${PRINT_OUT_COLOR}GCARD_FILE:${COLOR_END} ${GCARD_FILE}"

                # Check if GCARD_FILE is a file
                echo "${PRINT_OUT_COLOR}--> Checking if ${COLOR_END}GCARD_FILE${PRINT_OUT_COLOR} is a file...${COLOR_END}"
                if ( ! -f "${GCARD_FILE}" ) then
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following file does not exist: ${GCARD_FILE}"
                    exit 1
                else
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}GCARD_FILE exists.${COLOR_END}"
                    echo
                endif

                # Setting YAML_FILE
                unsetenv YAML_FILE
                if ("${TEMP_BEAM_E}" == "2070MeV") then
                    setenv YAML_FILE ${REQUIREMENTS_PATH}/rgm_fall2021-cv.yaml
                else if ("${TEMP_BEAM_E}" == "4029MeV") then
                    setenv YAML_FILE ${REQUIREMENTS_PATH}/rgm_fall2021-ai_4Gev.yaml
                else if ("${TEMP_BEAM_E}" == "5986MeV") then
                    setenv YAML_FILE ${REQUIREMENTS_PATH}/rgm_fall2021-ai_6Gev.yaml
                endif
                echo "${PRINT_OUT_COLOR}YAML_FILE:${COLOR_END} ${YAML_FILE}"

                # Check if YAML_FILE is a file
                echo "${PRINT_OUT_COLOR}--> Checking if ${COLOR_END}YAML_FILE${PRINT_OUT_COLOR} is a file...${COLOR_END}"
                if ( ! -f "${YAML_FILE}" ) then
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following file does not exist: ${YAML_FILE}"
                    exit 1
                else
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}YAML_FILE exists.${COLOR_END}"
                    echo
                endif

                # Setup output directory structure
                # ---------------------------------------------------------------------------
                echo "${PRINT_OUT_COLOR}Setting output directory structure${COLOR_END}"
                echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
                echo

                echo "${PRINT_OUT_COLOR}Removing old directory structure for MC simulation here...\033[0m"
                rm -rf ${OUTPATH}/mchipo
                rm -rf ${OUTPATH}/reconhipo
                echo

                echo "${PRINT_OUT_COLOR}Setting up directory structure for MC simulation here...\033[0m"
                mkdir -p ${OUTPATH}/lundfiles
                mkdir ${OUTPATH}/mchipo ${OUTPATH}/reconhipo
                echo

                # Check if the directories were created successfully
                echo "${PRINT_OUT_COLOR}Number of files in target directory (OUTPATH):${COLOR_END}"
                echo "${PRINT_OUT_COLOR}Number of lund files:     \t\t${COLOR_END} `ls ${OUTPATH}/lundfiles | wc -l`"
                echo "${PRINT_OUT_COLOR}Number of mchipo files:   \t\t${COLOR_END} `ls ${OUTPATH}/mchipo | wc -l`"
                echo "${PRINT_OUT_COLOR}Number of reconhipo files:\t\t${COLOR_END} `ls ${OUTPATH}/reconhipo | wc -l`"
                echo

                # Submitting job
                echo "${PRINT_OUT_COLOR}- Submitting jobs ------------------------------------------------------${COLOR_END}"
                echo ""
                echo "${PRINT_OUT_COLOR}Submitting GENIE sbatch job...${COLOR_END}"

                unsetenv SLURM_JOB_NAME
                setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENIE_TUNE}_${TEMP_BEAM_E}_${Q2_CUT}${FC_STATUS}
                echo "${PRINT_OUT_COLOR}SLURM_JOB_NAME:${COLOR_END} ${SLURM_JOB_NAME}"
                echo ""
                    
                unsetenv ARRAY
                setenv ARRAY 1-${NUM_OF_JOBS}
                echo "${PRINT_OUT_COLOR}ARRAY:${COLOR_END} ${ARRAY}"
                echo ""

                unsetenv SUBMIT_SCRIPT_FILE
                setenv SUBMIT_SCRIPT_FILE ${RUNNING_DIR}/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh
                echo "${PRINT_OUT_COLOR}SUBMIT_SCRIPT_FILE:${COLOR_END} ${SUBMIT_SCRIPT_FILE}"

                # Check if SUBMIT_SCRIPT_FILE is a file
                echo "${PRINT_OUT_COLOR}--> Checking if ${COLOR_END}SUBMIT_SCRIPT_FILE${PRINT_OUT_COLOR} is a file...${COLOR_END}"
                if ( ! -f "${SUBMIT_SCRIPT_FILE}" ) then
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following file does not exist: ${SUBMIT_SCRIPT_FILE}"
                    exit 1
                else
                    printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}SUBMIT_SCRIPT_FILE exists.${COLOR_END}"
                    echo
                endif

                echo "${PRINT_OUT_COLOR}Submitted job with command:${COLOR_END}"
                echo "${PRINT_OUT_COLOR}sbatch --job-name=${COLOR_END}${SLURM_JOB_NAME}${PRINT_OUT_COLOR} --array=${COLOR_END}${ARRAY} ${SUBMIT_SCRIPT_FILE}"
                # sbatch --job-name="${SLURM_JOB_NAME}" --array=${ARRAY} ${SUBMIT_SCRIPT_FILE}
                echo
                echo
            end  # End of loop over beam energies
        end  # End of loop over GENIE tunes
    end  # End of loop over target nuclei
end  # End of loop over fiducial cuts statuses
