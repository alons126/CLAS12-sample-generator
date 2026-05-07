#!/bin/csh

# Wrapper for the ROOT GENIE-to-LUND conversion workflow.
# This script is intended to be run from inside GEMC-samples/. It sets
# the truth-sample input location and target / beam configuration, checks
# that the expected ROOT inputs exist, and then invokes
# GENIE_to_LUND_converter/GENIE_to_LUND_converter.C.

# Setup environment
# ======================================================================================================
source ./scripts/set_env.csh
echo

echo
echo "\033[35m------------------ GENIE to LUND converter ------------------\033[0m"
echo
echo "\033[35m----------------------- Job parameters ----------------------\033[0m"
echo

# Set number of files
unsetenv NUM_OF_FILES
# setenv NUM_OF_FILES 1000
# setenv NUM_OF_FILES 2500
setenv NUM_OF_FILES 10000
echo "\033[35mNUM_OF_FILES:\033[0m ${NUM_OF_FILES}"
echo ""

# Set base directory
unsetenv BASE_TL_SAMPLE_DIR
setenv BASE_TL_SAMPLE_DIR /w/hallb-scshelf2102/clas12/asportes/2N_Analysis_Truth_Samples
echo "\033[35mBASE_TL_SAMPLE_DIR:\033[0m ${BASE_TL_SAMPLE_DIR}"
echo ""

# Check if BASE_TL_SAMPLE_DIR is a directory
echo "${COLOR_START}--> Checking if ${COLOR_END}BASE_TL_SAMPLE_DIR${COLOR_START} is a directory...${COLOR_END}"
if ( ! -d "${BASE_TL_SAMPLE_DIR}" ) then
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${BASE_TL_SAMPLE_DIR}"
    exit 1
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}BASE_TL_SAMPLE_DIR exists.${COLOR_END}"
    echo
endif

# Set target nucleus
unsetenv TL_SAMPLE_TARGET_NUCLEUS
# setenv TL_SAMPLE_TARGET_NUCLEUS H1
# setenv TL_SAMPLE_TARGET_NUCLEUS D2
setenv TL_SAMPLE_TARGET_NUCLEUS C12
# setenv TL_SAMPLE_TARGET_NUCLEUS Ar40
echo "\033[35mTL_SAMPLE_TARGET_NUCLEUS:\033[0m ${TL_SAMPLE_TARGET_NUCLEUS}"
echo ""

# Set Z and A for selected target
unsetenv TL_SAMPLE_TARGET_NUCLEUS_A
unsetenv TL_SAMPLE_TARGET_NUCLEUS_Z

if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "H1") then
    setenv TL_SAMPLE_TARGET_NUCLEUS_A 1
    setenv TL_SAMPLE_TARGET_NUCLEUS_Z 1
else if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "D2") then
    setenv TL_SAMPLE_TARGET_NUCLEUS_A 2
    setenv TL_SAMPLE_TARGET_NUCLEUS_Z 1
else if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "C12") then
    setenv TL_SAMPLE_TARGET_NUCLEUS_A 12
    setenv TL_SAMPLE_TARGET_NUCLEUS_Z 6
else if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "Ar40") then
    setenv TL_SAMPLE_TARGET_NUCLEUS_A 40
    setenv TL_SAMPLE_TARGET_NUCLEUS_Z 18
else
    echo "\033[31mError:\033[0m unknown target: ${TL_SAMPLE_TARGET_NUCLEUS}"
    exit 1
endif

echo "\033[35mTL_SAMPLE_TARGET_NUCLEUS_A:\033[0m ${TL_SAMPLE_TARGET_NUCLEUS_A}"
echo "\033[35mTL_SAMPLE_TARGET_NUCLEUS_Z:\033[0m ${TL_SAMPLE_TARGET_NUCLEUS_Z}"
echo ""

# Set GENIE tune
unsetenv TL_GENIE_TUNE
setenv TL_GENIE_TUNE G18_10a_00_000
# setenv TL_GENIE_TUNE GEM21_11a_00_000
echo "\033[35mTL_GENIE_TUNE:\033[0m ${TL_GENIE_TUNE}"
echo ""

# Set beam energy
unsetenv TL_SAMPLE_ENERGY
setenv TL_SAMPLE_ENERGY 2070MeV
# setenv TL_SAMPLE_ENERGY 4029MeV
# setenv TL_SAMPLE_ENERGY 5986MeV
echo "\033[35mTL_SAMPLE_ENERGY:\033[0m ${TL_SAMPLE_ENERGY}"
echo ""

# Set target type
# Relevant for GENIE-to-LUND conversion since it determines the target geometry and thus the acceptance cuts. Used with the targets.h header in GENIE_to_LUND_converter.C.
unsetenv TL_SAMPLE_TARGET_TYPE

if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "H1" || "${TL_SAMPLE_TARGET_NUCLEUS}" == "D2") then
    setenv TL_SAMPLE_TARGET_TYPE liquid
else if ("${TL_SAMPLE_ENERGY}" == "2070MeV" && "${TL_SAMPLE_TARGET_NUCLEUS}" == "C12") then
    setenv TL_SAMPLE_TARGET_TYPE 1-foil-small
else if ("${TL_SAMPLE_ENERGY}" == "4029MeV" && "${TL_SAMPLE_TARGET_NUCLEUS}" == "C12") then
    setenv TL_SAMPLE_TARGET_TYPE 1-foil-large
else if ("${TL_SAMPLE_ENERGY}" == "5986MeV" && "${TL_SAMPLE_TARGET_NUCLEUS}" == "C12") then
    setenv TL_SAMPLE_TARGET_TYPE 4-foil
else if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "Ar40") then
    setenv TL_SAMPLE_TARGET_TYPE Ar
else
    echo "\033[31mError:\033[0m unknown target type for energy/target: ${TL_SAMPLE_ENERGY}, ${TL_SAMPLE_TARGET_NUCLEUS}"
    exit 1
endif

echo "\033[35mTL_SAMPLE_TARGET_TYPE:\033[0m ${TL_SAMPLE_TARGET_TYPE}"
echo ""

# Set Q² cut based on energy
unsetenv TL_SAMPLE_Q2_CUT
if ("${TL_SAMPLE_ENERGY}" == "2070MeV") then
    setenv TL_SAMPLE_Q2_CUT Q2_0_02
else if ("${TL_SAMPLE_ENERGY}" == "4029MeV") then
    setenv TL_SAMPLE_Q2_CUT Q2_0_25
else if ("${TL_SAMPLE_ENERGY}" == "5986MeV") then
    setenv TL_SAMPLE_Q2_CUT Q2_0_40
else
    echo "\033[31mError:\033[0m unknown beam energy: ${TL_SAMPLE_ENERGY}"
    exit 1
endif

echo "\033[35mTL_SAMPLE_Q2_CUT:\033[0m ${TL_SAMPLE_Q2_CUT}"
echo ""

# Safety check: check if directory exists
set INPUT_FILES_DIR_PATH = "${BASE_TL_SAMPLE_DIR}/${TL_SAMPLE_TARGET_NUCLEUS}/${TL_GENIE_TUNE}/${TL_SAMPLE_ENERGY}_${TL_SAMPLE_Q2_CUT}/master-routine_validation_01-eScattering"

# Check if INPUT_FILES_DIR_PATH is a directory
echo "${COLOR_START}--> Checking if ${COLOR_END}INPUT_FILES_DIR_PATH${COLOR_START} is a directory...${COLOR_END}"
if ( ! -d "${INPUT_FILES_DIR_PATH}" ) then
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following directory does not exist: ${INPUT_FILES_DIR_PATH}"
    exit 1
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_GOOD_START}INPUT_FILES_DIR_PATH exists.${COLOR_END}"
    echo
endif

# Expand root files using shell globbing
set INPUT_FILES_PATTERN = "${BASE_TL_SAMPLE_DIR}/${TL_SAMPLE_TARGET_NUCLEUS}/${TL_GENIE_TUNE}/${TL_SAMPLE_ENERGY}_${TL_SAMPLE_Q2_CUT}/master-routine_validation_01-eScattering/*.root"

# Expand into array
set INPUT_FILES_DIR = ( ${INPUT_FILES_DIR_PATH}/*.root )

# Safety check: check if array is empty
if ( $#INPUT_FILES_DIR == 0 ) then
    echo "\033[31mError:\033[0m No .root files found in: ${INPUT_FILES_DIR_PATH}"
    exit 1
endif

# Convert to space-separated string
set INPUT_FILES_STRING = "`echo ${INPUT_FILES_DIR}`"
echo ""

# Execute ROOT macro
root -q -b './GENIE_to_LUND_converter/GENIE_to_LUND_converter.C("'"${INPUT_FILES_PATTERN}"'", '"${NUM_OF_FILES}"', "'"${TL_SAMPLE_TARGET_TYPE}"'", '"${TL_SAMPLE_TARGET_NUCLEUS_A}"', '"${TL_SAMPLE_TARGET_NUCLEUS_Z}"')'

echo
echo "\033[35m----------------------- End of job --------------------------\033[0m"
echo
