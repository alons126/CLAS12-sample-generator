#!/bin/csh

echo
echo "------------------ GENIE to LUND converter ------------------"
echo
echo "----------------------- Job parameters ----------------------"
echo

# Set number of files
unsetenv NUM_OF_FILES
setenv NUM_OF_FILES 2500
# setenv NUM_OF_FILES 10
echo "NUM_OF_FILES = ${NUM_OF_FILES}"
echo ""

# Set base directory
unsetenv BASE_TL_SAMPLE_DIR
setenv BASE_TL_SAMPLE_DIR /w/hallb-scshelf2102/clas12/asportes/2N_Analysis_Truth_Samples
echo "BASE_TL_SAMPLE_DIR = ${BASE_TL_SAMPLE_DIR}"
echo ""

# Set target nucleus
unsetenv TL_SAMPLE_TARGET_NUCLEUS
# setenv TL_SAMPLE_TARGET_NUCLEUS H1
# setenv TL_SAMPLE_TARGET_NUCLEUS D2
setenv TL_SAMPLE_TARGET_NUCLEUS C12
# setenv TL_SAMPLE_TARGET_NUCLEUS Ar40
echo "TL_SAMPLE_TARGET_NUCLEUS = ${TL_SAMPLE_TARGET_NUCLEUS}"
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
    echo "Unknown target: ${TL_SAMPLE_TARGET_NUCLEUS}"
    exit 1
endif

echo "TL_SAMPLE_TARGET_NUCLEUS_A = ${TL_SAMPLE_TARGET_NUCLEUS_A}"
echo "TL_SAMPLE_TARGET_NUCLEUS_Z = ${TL_SAMPLE_TARGET_NUCLEUS_Z}"
echo ""

# Set GENIE tune
unsetenv TL_GENIE_TUNE
# setenv TL_GENIE_TUNE G18_10a_00_000
setenv TL_GENIE_TUNE GEM21_11a_00_000
echo "TL_GENIE_TUNE = ${TL_GENIE_TUNE}"
echo ""

# Set beam energy
unsetenv TL_SAMPLE_ENERGY
# setenv TL_SAMPLE_ENERGY 2070MeV
setenv TL_SAMPLE_ENERGY 4029MeV
# setenv TL_SAMPLE_ENERGY 5986MeV
echo "TL_SAMPLE_ENERGY = ${TL_SAMPLE_ENERGY}"
echo ""

# Set target type
unsetenv TL_SAMPLE_TARGET_TYPE

if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "H1" || "${TL_SAMPLE_TARGET_NUCLEUS}" == "D2") then
    setenv TL_SAMPLE_TARGET_TYPE liquid
else if (("${TL_SAMPLE_ENERGY}" == "2070MeV" || "${TL_SAMPLE_ENERGY}" == "4029MeV") && "${TL_SAMPLE_TARGET_NUCLEUS}" == "C12") then
    setenv TL_SAMPLE_TARGET_TYPE 1-foil
else if ("${TL_SAMPLE_ENERGY}" == "5986MeV" && "${TL_SAMPLE_TARGET_NUCLEUS}" == "C12") then
    setenv TL_SAMPLE_TARGET_TYPE 4-foil
else if ("${TL_SAMPLE_TARGET_NUCLEUS}" == "Ar40") then
    setenv TL_SAMPLE_TARGET_TYPE Ar
else
    echo "Unknown target type for energy/target: ${TL_SAMPLE_ENERGY}, ${TL_SAMPLE_TARGET_NUCLEUS}"
    exit 1
endif

echo "TL_SAMPLE_TARGET_TYPE = ${TL_SAMPLE_TARGET_TYPE}"
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
    echo "Unknown beam energy: ${TL_SAMPLE_ENERGY}"
    exit 1
endif

echo "TL_SAMPLE_Q2_CUT = ${TL_SAMPLE_Q2_CUT}"
echo ""

# Expand root files using shell globbing
set INPUT_FILES_PATTERN = "${BASE_TL_SAMPLE_DIR}/${TL_SAMPLE_TARGET_NUCLEUS}/${TL_GENIE_TUNE}/${TL_SAMPLE_ENERGY}_${TL_SAMPLE_Q2_CUT}/master-routine_validation_01-eScattering/*.root"
set INPUT_FILES_DIR = ( $BASE_TL_SAMPLE_DIR/$TL_SAMPLE_TARGET_NUCLEUS/$TL_GENIE_TUNE/${TL_SAMPLE_ENERGY}_${TL_SAMPLE_Q2_CUT}/master-routine_validation_01-eScattering/*.root )
set INPUT_FILES_STRING = "`echo $INPUT_FILES_DIR`"
# echo "INPUT_FILES_DIR = ${INPUT_FILES_DIR}"
echo ""

# Execute ROOT macro
root -q -b './GENIE_to_LUND_converter/GENIE_to_LUND_converter.C("'"${INPUT_FILES_PATTERN}"'", '"${NUM_OF_FILES}"', "'"${TL_SAMPLE_TARGET_TYPE}"'", '"${TL_SAMPLE_TARGET_NUCLEUS_A}"', '"${TL_SAMPLE_TARGET_NUCLEUS_Z}"')'

echo
echo "----------------------- End of job --------------------------"
echo
