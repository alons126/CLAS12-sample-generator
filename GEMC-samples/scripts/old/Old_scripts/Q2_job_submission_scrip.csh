#!/bin/csh

# setenv BEAM_E 4029MeV
setenv BEAM_E 5986MeV
echo "BEAM_E:\t\t\t${BEAM_E}"

# setenv GENIE_TUNE G18_10a_00_000
setenv GENIE_TUNE GEM21_11a_00_000
echo "GENIE_TUNE:\t\t${GENIE_TUNE}"

if ("${BEAM_E}" == "4029MeV") then
    setenv TARGET C12
    echo "TARGET:\t\t\t${TARGET}"
else if ("${BEAM_E}" == "5986MeV") then
    setenv TARGET Ar40
    echo "TARGET:\t\t\t${TARGET}"
endif

setenv CLEAR_FARM_OUT "false"
echo "CLEAR_FARM_OUT:\t\t${CLEAR_FARM_OUT}"
echo

# Determine the correct submit script path based on BEAM_E
setenv SUBMIT_SCRIPT_PATH ./${TARGET}_Q2_sample_${BEAM_E}/
echo "SUBMIT_SCRIPT_PATH:\t${SUBMIT_SCRIPT_PATH}"
echo

# Optionally clear the farm_out directory
if ("${CLEAR_FARM_OUT}" == "true") then
    echo
    echo "Clearing farm_out directory..."
    rm /u/scifarm/farm_out/asportes/*
    echo
endif

if ("${BEAM_E}" == "4029MeV") then
# Loop over directories and submit jobs
foreach TEMP_Q2_CUT ( \
            Q2_0_02 Q2_0_03 Q2_0_04 Q2_0_05 Q2_0_06 Q2_0_07 Q2_0_08 Q2_0_09 Q2_0_10 Q2_0_11 \
            Q2_0_12 Q2_0_13 Q2_0_14 Q2_0_15 Q2_0_16 Q2_0_17 Q2_0_18 Q2_0_19 Q2_0_20 Q2_0_21 \
            Q2_0_22 Q2_0_23 Q2_0_24 Q2_0_25 Q2_0_26 Q2_0_27 Q2_0_28 Q2_0_29 Q2_0_30 Q2_0_31 \
            Q2_0_32 Q2_0_33 Q2_0_34 Q2_0_35 Q2_0_36 Q2_0_37 Q2_0_38 Q2_0_39 Q2_0_40 )
    echo "- Submitting ${TARGET}_${GENIE_TUNE}_${BEAM_E} jobs ------------"
    echo

    # Construct the full JOB_OUT_PATH
    setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/${TARGET}/${GENIE_TUNE}/Q2_th_test_samples/${BEAM_E}/${TEMP_Q2_CUT}
    echo "JOB_OUT_PATH:\t\t${JOB_OUT_PATH}"
    echo "TEMP_Q2_CUT:\t\t${TEMP_Q2_CUT}"
    echo

    echo "Removing old directory structure for MC simulation..."
    rm -rf ${JOB_OUT_PATH}/mchipo ${JOB_OUT_PATH}/reconhipo ${JOB_OUT_PATH}/rootfiles
    echo

    echo "Setting up directory structure for MC simulation..."
    mkdir -p ${JOB_OUT_PATH}/mchipo ${JOB_OUT_PATH}/reconhipo ${JOB_OUT_PATH}/rootfiles
    echo

    echo "Submitting sbatch jobs for ${TARGET} at BeamE = ${BEAM_E} with a ${TEMP_Q2_CUT} cut..."
    sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_Q2.sh
    echo
end
else if ("${BEAM_E}" == "5986MeV") then
foreach TEMP_Q2_CUT ( \
            Q2_0_40 Q2_0_41 Q2_0_42 Q2_0_43 Q2_0_44 Q2_0_45 Q2_0_46 Q2_0_47 Q2_0_48 Q2_0_49 \
            Q2_0_50 Q2_0_51 Q2_0_52 Q2_0_53 Q2_0_54 Q2_0_55 Q2_0_56 Q2_0_57 Q2_0_58 Q2_0_59 \
            Q2_0_60 Q2_0_61 Q2_0_62 Q2_0_63 Q2_0_64 Q2_0_65 Q2_0_66 Q2_0_67 Q2_0_68 Q2_0_69 \
            Q2_0_70 Q2_0_71 Q2_0_72 Q2_0_73 Q2_0_74 Q2_0_75 Q2_0_76 Q2_0_77 Q2_0_78 Q2_0_79 \
            Q2_0_80 )
    echo "- Submitting ${TARGET}_${GENIE_TUNE}_${BEAM_E} jobs ------------"
    echo

    # Construct the full JOB_OUT_PATH
    setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/${TARGET}/${GENIE_TUNE}/Q2_th_test_samples/${BEAM_E}/${TEMP_Q2_CUT}
    echo "JOB_OUT_PATH:\t\t${JOB_OUT_PATH}"
    echo "TEMP_Q2_CUT:\t\t${TEMP_Q2_CUT}"
    echo

    echo "Removing old directory structure for MC simulation..."
    rm -rf ${JOB_OUT_PATH}/mchipo ${JOB_OUT_PATH}/reconhipo ${JOB_OUT_PATH}/rootfiles
    echo

    echo "Setting up directory structure for MC simulation..."
    mkdir -p ${JOB_OUT_PATH}/mchipo ${JOB_OUT_PATH}/reconhipo ${JOB_OUT_PATH}/rootfiles
    echo

    echo "Submitting sbatch jobs for ${TARGET} at BeamE = ${BEAM_E} with a ${TEMP_Q2_CUT} cut..."
    sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_Q2.sh
    echo
end
endif