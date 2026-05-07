#!/bin/csh

# # Example of calling the function with a beam energy and enabling farm_out clearing
# uniform_setup_and_submit "5986MeV" true # Won’t clear farm_out.
# uniform_setup_and_submit "5986MeV"      # Will clear farm_out.

function uniform_setup_and_submit (BEAM_E, clear_farm_out = false) {
    # Set paths based on BEAM_E
    # setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}
    setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}_target_zpos_test
    # setenv JOB_OUT_PATH_1E ${JOB_OUT_PATH}/OutPut_1e
    # setenv JOB_OUT_PATH_EP ${JOB_OUT_PATH}/OutPut_ep
    setenv JOB_OUT_PATH_EN ${JOB_OUT_PATH}/OutPut_en

    # Determine the correct submit script path based on BEAM_E
    if (${BEAM_E} == "5986MeV") then
        setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_6GeV/
    else if (${BEAM_E} == "4029MeV") then
        setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_4GeV/
    else if (${BEAM_E} == "2070MeV") then
        setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_2GeV/
    endif

    echo
    echo "Pulling updates..."
    git pull
    echo

    # Optionally clear the farm_out directory
    if (${clear_farm_out}) then
        echo
        echo "Clearing farm_out directory..."
        rm /u/scifarm/farm_out/asportes/*
        echo
    endif

    echo
    echo "Removing old directory structure for MC simulation here..."
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

    echo
    echo "Setting up directory structure for MC simulation here..."
    # mkdir ${JOB_OUT_PATH_1E}/mchipo ${JOB_OUT_PATH_1E}/reconhipo ${JOB_OUT_PATH_1E}/rootfiles
    # mkdir ${JOB_OUT_PATH_EP}/mchipo ${JOB_OUT_PATH_EP}/reconhipo ${JOB_OUT_PATH_EP}/rootfiles
    mkdir ${JOB_OUT_PATH_EN}/mchipo ${JOB_OUT_PATH_EN}/reconhipo ${JOB_OUT_PATH_EN}/rootfiles
    echo

    # echo
    # echo "Submitting 1e sbatch job for BeamE = ${BEAM_E}..."
    # sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_1e.sh
    # echo

    # echo "Submitting ep sbatch job for BeamE = ${BEAM_E}..."
    # sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_ep.sh
    # echo

    echo "Submitting en sbatch job for BeamE = ${BEAM_E}..."
    sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_en.sh
    echo
}


# # Function implementation without parentheses
# alias uniform_setup_and_submit 'uniform_setup_and_submit_impl \!*'

# # Function implementation
# uniform_setup_and_submit_impl:
#     set BEAM_E = $1
#     set clear_farm_out = $2

#     # Set paths based on BEAM_E
#     setenv JOB_OUT_PATH /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/Uniform_e-p-n_samples/${BEAM_E}
#     setenv JOB_OUT_PATH_1E ${JOB_OUT_PATH}/OutPut_1e
#     setenv JOB_OUT_PATH_EP ${JOB_OUT_PATH}/OutPut_ep
#     setenv JOB_OUT_PATH_EN ${JOB_OUT_PATH}/OutPut_en

#     # Determine the correct submit script path based on BEAM_E
#     if (${BEAM_E} == "5986MeV") then
#         setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_6GeV/
#     else if (${BEAM_E} == "4029MeV") then
#         setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_4GeV/
#     else if (${BEAM_E} == "2070MeV") then
#         setenv SUBMIT_SCRIPT_PATH ./Uniform_sample_2GeV/
#     endif

#     echo
#     echo "Pulling updates..."
#     git pull
#     echo

#     # Optionally clear the farm_out directory
#     if ("$clear_farm_out" == "true") then
#         echo
#         echo "Clearing farm_out directory..."
#         rm /u/scifarm/farm_out/asportes/*
#         echo
#     endif

#     echo
#     echo "Removing old directory structure for MC simulation here..."
#     rm -rf ${JOB_OUT_PATH_1E}/mchipo
#     rm -rf ${JOB_OUT_PATH_1E}/reconhipo
#     rm -rf ${JOB_OUT_PATH_1E}/rootfiles

#     rm -rf ${JOB_OUT_PATH_EP}/mchipo
#     rm -rf ${JOB_OUT_PATH_EP}/reconhipo
#     rm -rf ${JOB_OUT_PATH_EP}/rootfiles

#     rm -rf ${JOB_OUT_PATH_EN}/mchipo
#     rm -rf ${JOB_OUT_PATH_EN}/reconhipo
#     rm -rf ${JOB_OUT_PATH_EN}/rootfiles
#     echo

#     echo
#     echo "Setting up directory structure for MC simulation here..."
#     mkdir ${JOB_OUT_PATH_1E}/mchipo ${JOB_OUT_PATH_1E}/reconhipo ${JOB_OUT_PATH_1E}/rootfiles
#     mkdir ${JOB_OUT_PATH_EP}/mchipo ${JOB_OUT_PATH_EP}/reconhipo ${JOB_OUT_PATH_EP}/rootfiles
#     mkdir ${JOB_OUT_PATH_EN}/mchipo ${JOB_OUT_PATH_EN}/reconhipo ${JOB_OUT_PATH_EN}/rootfiles
#     echo

#     echo
#     echo "Submitting 1e sbatch job for BeamE = ${BEAM_E}..."
#     sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_1e.sh
#     echo

#     echo "Submitting ep sbatch job for BeamE = ${BEAM_E}..."
#     sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_ep.sh
#     echo

#     echo "Submitting en sbatch job for BeamE = ${BEAM_E}..."
#     sbatch ${SUBMIT_SCRIPT_PATH}/submit_GEMC_uniform_en.sh
#     echo
