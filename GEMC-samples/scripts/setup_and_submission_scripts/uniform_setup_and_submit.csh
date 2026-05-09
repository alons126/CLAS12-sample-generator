#!/bin/csh

# A script to setup environment variables and paths for uniform sample generation and submission, and then submit the jobs to the cluster using sbatch. This script is designed to be flexible and configurable through environment variables, allowing for easy adjustments to the simulation parameters, output paths, and other settings without needing to modify the core logic of the script. The script also includes checks to ensure that the necessary directories and files exist before attempting to submit the jobs, providing error messages if any required resources are missing. The uniform sample generation and submission is performed for different beam energies and particle types, with the appropriate configurations and paths set for each case.

# Setup environment
# ======================================================================================================
source ./scripts/set_env.csh
echo

# Script banner
# ======================================================================================================
echo ""
echo "${SYSTEM_COLOR}///////////////////////////////////////////////////////////////////////${COLOR_END}"
printf "%s%s%s\n" "${SYSTEM_COLOR}//${COLOR_END}        Setting and submitting uniform sample generation jobs      ${SYSTEM_COLOR}//${COLOR_END}"
echo "${SYSTEM_COLOR}///////////////////////////////////////////////////////////////////////${COLOR_END}"
echo ""

# Setup environment variables and paths for uniform sample generation and submission
# ======================================================================================================
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s\n" "${COLOR_START}= Setup environment variables and paths                               =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

unset TARGET_VARIATION
setenv TARGET_VARIATION rgm_fall2021_Ar
echo "${COLOR_START}TARGET_VARIATION:${COLOR_END}    ${TARGET_VARIATION}"
echo

unsetenv CLEAR_FAR_OUT
setenv CLEAR_FAR_OUT false
echo "${COLOR_START}CLEAR_FAR_OUT:${COLOR_END}       ${CLEAR_FAR_OUT}"
echo

unset CUSTOM_GEMC_VERSION
setenv CUSTOM_GEMC_VERSION true
echo "${COLOR_START}CUSTOM_GEMC_VERSION:${COLOR_END} ${CUSTOM_GEMC_VERSION}"
echo

unset GEMC_VERSION
# setenv GEMC_VERSION dev
setenv GEMC_VERSION 5.14
echo "${COLOR_START}GEMC_VERSION:${COLOR_END}        ${GEMC_VERSION}"
echo

unset NUM_OF_JOBS
# setenv NUM_OF_JOBS 10
# setenv NUM_OF_JOBS 100
# setenv NUM_OF_JOBS 2500
setenv NUM_OF_JOBS 5000
# setenv NUM_OF_JOBS 7500
# setenv NUM_OF_JOBS 10000
echo "${COLOR_START}NUM_OF_JOBS:${COLOR_END}         ${NUM_OF_JOBS}"
echo

# Main Script
# ======================================================================================================
echo ""
echo "${COLOR_START}=======================================================================${COLOR_END}"
printf "%s%s%s\n" "${COLOR_START}= " "Starting uniform generation and submission         ${COLOR_START}=${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""

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
    # unsetenv GEMC_DATA_DIR
    # setenv GEMC_DATA_DIR ${CLAS12TAGS_DIR}
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
printf "%s%s%s\n" "${COLOR_START}= Looping over particle types                                         =${COLOR_END}"
echo "${COLOR_START}=======================================================================${COLOR_END}"
echo ""
# foreach BEAM_E ( 2070MeV )
# foreach BEAM_E ( 4029MeV 5986MeV )
foreach BEAM_E ( 2070MeV 4029MeV 5986MeV )
    foreach OUTPATH_PARTICLE ( 1e )
    # foreach OUTPATH_PARTICLE ( en )
    # foreach OUTPATH_PARTICLE ( 1e en )
    # foreach OUTPATH_PARTICLE ( 1e ep en )
        echo
        echo "${COLOR_START}Processing particle type ${COLOR_END}${OUTPATH_PARTICLE}${COLOR_START} at beam energy ${COLOR_END}${BEAM_E}"
        echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
        echo

        # Defining a temporary environment variable for beam energy. This is crucial for the job submission script, as BEAM_E is not accepted.
        unsetenv TEMP_BEAM_E
        setenv TEMP_BEAM_E ${BEAM_E}
        echo "${COLOR_START}TEMP_BEAM_E:${COLOR_END} ${TEMP_BEAM_E}"
        echo

        # Defining a temporary environment variable for particle type. This is crucial for the job submission script, as OUTPATH_PARTICLE is not accepted.
        unsetenv TEMP_OUTPATH_PARTICLE
        setenv TEMP_OUTPATH_PARTICLE ${OUTPATH_PARTICLE}
        echo "${COLOR_START}TEMP_OUTPATH_PARTICLE:${COLOR_END} ${TEMP_OUTPATH_PARTICLE}"
        echo

        # Set printing color based on beam energy and fiducial cuts status
        unsetenv PRINT_OUT_COLOR
        if ("${TEMP_BEAM_E}" == "2070MeV" && "${TEMP_OUTPATH_PARTICLE}" == "1e") then
            setenv PRINT_OUT_COLOR "`printf '\033[34m'`"   # blue
        else if ("${TEMP_BEAM_E}" == "2070MeV" && "${TEMP_OUTPATH_PARTICLE}" == "en") then
            setenv PRINT_OUT_COLOR "`printf '\033[36m'`"   # cyan
        else if ("${TEMP_BEAM_E}" == "2070MeV" && "${TEMP_OUTPATH_PARTICLE}" == "ep") then
            setenv PRINT_OUT_COLOR "`printf '\033[37m'`"   # white

        else if ("${TEMP_BEAM_E}" == "4029MeV" && "${TEMP_OUTPATH_PARTICLE}" == "1e") then
            setenv PRINT_OUT_COLOR "`printf '\033[90m'`"   # bright black (gray)
        else if ("${TEMP_BEAM_E}" == "4029MeV" && "${TEMP_OUTPATH_PARTICLE}" == "ep") then
            setenv PRINT_OUT_COLOR "`printf '\033[91m'`"   # bright red
        else if ("${TEMP_BEAM_E}" == "4029MeV" && "${TEMP_OUTPATH_PARTICLE}" == "en") then
            setenv PRINT_OUT_COLOR "`printf '\033[92m'`"   # bright green

        else if ("${TEMP_BEAM_E}" == "5986MeV" && "${TEMP_OUTPATH_PARTICLE}" == "1e") then
            setenv PRINT_OUT_COLOR "`printf '\033[93m'`"   # bright yellow
        else if ("${TEMP_BEAM_E}" == "5986MeV" && "${TEMP_OUTPATH_PARTICLE}" == "ep") then
            setenv PRINT_OUT_COLOR "`printf '\033[94m'`"   # bright blue
        else if ("${TEMP_BEAM_E}" == "5986MeV" && "${TEMP_OUTPATH_PARTICLE}" == "en") then
            setenv PRINT_OUT_COLOR "`printf '\033[96m'`"   # bright cyan
        else
            echo "Unknown combination: TEMP_BEAM_E=${TEMP_BEAM_E}, TEMP_OUTPATH_PARTICLE=${TEMP_OUTPATH_PARTICLE}"
            exit 1
        endif

        # Set paths based on TEMP_BEAM_E and TARGET_VARIATION for uniform sample generation and submission. These environment variables will be used in the uniform sample generation and submission scripts to ensure that the correct paths and configurations are used for each beam energy and target variation.
        unsetenv OUTPATH_BASE
        setenv OUTPATH_BASE /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/${TEMP_BEAM_E}_GEMC5.14_rgm_fall2021_C_S
        # setenv OUTPATH_BASE /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/${TEMP_BEAM_E}_devGEMC_${TARGET_VARIATION}
        # setenv OUTPATH_BASE /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/${TEMP_BEAM_E}_ConstPn_devGEMC_${TARGET_VARIATION}
        echo "${PRINT_OUT_COLOR}OUTPATH_BASE: ${COLOR_END}${OUTPATH_BASE}"

        # Check if OUTPATH_BASE is a directory
        echo "${PRINT_OUT_COLOR}--> Checking if ${COLOR_END}OUTPATH_BASE${PRINT_OUT_COLOR} is a directory...${COLOR_END}"
        if ( ! -d "${OUTPATH_BASE}" ) then
            printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${OUTPATH_BASE}"
            exit 1
        else
            printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}OUTPATH_BASE exists.${COLOR_END}"
            echo
        endif

        # Setup other environment variables based on TEMP_BEAM_E and particle type
        # ---------------------------------------------------------------------------

        echo "${PRINT_OUT_COLOR}TEMP_OUTPATH_PARTICLE:${COLOR_END} ${TEMP_OUTPATH_PARTICLE}"
        echo

        # Setup environment variables based on TEMP_BEAM_E and particle type
        # ---------------------------------------------------------------------------
        echo "${PRINT_OUT_COLOR}Setting environment variables based on TEMP_BEAM_E and particle type ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo

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
            echo "${COLOR_ERROR_START}Error:${COLOR_END} unknown torus field configuration: ${TEMP_BEAM_E}"
            exit 1
        endif

        # Set paths based on particle type
        # --------------------------------------------------------------------------------------------------
        echo "${PRINT_OUT_COLOR}Setting paths based on particle type ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo

        unsetenv OUTPATH
        setenv OUTPATH ${OUTPATH_BASE}/OutPut_${TEMP_OUTPATH_PARTICLE}
        echo "${PRINT_OUT_COLOR}OUTPATH:${COLOR_END} ${OUTPATH}"

        # Check if OUTPATH is a directory
        echo "${PRINT_OUT_COLOR}--> Checking if ${COLOR_END}OUTPATH${PRINT_OUT_COLOR} is a directory...${COLOR_END}"
        if ( ! -d "${OUTPATH}" ) then
            printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${OUTPATH}"
            exit 1
        else
            printf "${PRINT_OUT_COLOR}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}OUTPATH exists.${COLOR_END}"
            echo
        endif

        # Determine the correct submit script path based on TEMP_BEAM_E
        unsetenv REQUIREMENTS_PATH
        setenv REQUIREMENTS_PATH ./Generation_files_${TEMP_BEAM_E_ROUNDED}
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
        echo "${PRINT_OUT_COLOR}Setting GCARD_FILE and YAML_FILE files based on TEMP_BEAM_E and TARGET_VARIATION ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
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
        echo "${PRINT_OUT_COLOR}Setting output directory structure ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo

        echo "${PRINT_OUT_COLOR}Removing old directory structure for MC simulation here...\033[0m"
        rm -rf ${OUTPATH}/mchipo
        rm -rf ${OUTPATH}/reconhipo
        rm -rf ${OUTPATH}/rootfiles
        echo

        echo "${PRINT_OUT_COLOR}Setting up directory structure for MC simulation here...\033[0m"
        mkdir ${OUTPATH}/mchipo ${OUTPATH}/reconhipo ${OUTPATH}/rootfiles
        echo

        # Check if the directories were created successfully
        echo "${PRINT_OUT_COLOR}Number of files in target directory (OUTPATH):${COLOR_END}"
        echo "${PRINT_OUT_COLOR}Number of lund files:     \t\t${COLOR_END} `ls ${OUTPATH}/lundfiles | wc -l`"
        echo "${PRINT_OUT_COLOR}Number of mchipo files:   \t\t${COLOR_END} `ls ${OUTPATH}/mchipo | wc -l`"
        echo "${PRINT_OUT_COLOR}Number of reconhipo files:\t\t${COLOR_END} `ls ${OUTPATH}/reconhipo | wc -l`"
        echo

        # Submitting sbatch job
        # ---------------------------------------------------------------------------
        echo "${PRINT_OUT_COLOR}Submitting sbatch job for BeamE = ${COLOR_END}${TEMP_BEAM_E}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo

        unsetenv SLURM_JOB_NAME
        setenv SLURM_JOB_NAME Uniform_${TEMP_OUTPATH_PARTICLE}_sample_${TEMP_BEAM_E}
        # setenv SLURM_JOB_NAME Uniform_${TEMP_OUTPATH_PARTICLE}_ConstPn_sample_${TEMP_BEAM_E}
        echo "${PRINT_OUT_COLOR}SLURM_JOB_NAME:${COLOR_END} ${SLURM_JOB_NAME}"
        echo ""

        unsetenv ARRAY
        setenv ARRAY 1-${NUM_OF_JOBS}
        echo "${PRINT_OUT_COLOR}ARRAY:${COLOR_END} ${ARRAY}"
        echo ""

        unsetenv SUBMIT_SCRIPT_FILE
        setenv SUBMIT_SCRIPT_FILE ./scripts/job_submission_scripts/submit_GEMC_uniform_sample.sh
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
    end # end foreach OUTPATH_PARTICLE ( 1e ep en )
end # end foreach BEAM_E ( 2070MeV 4029MeV 5986MeV )
