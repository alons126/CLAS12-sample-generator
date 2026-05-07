#!/bin/csh

echo "\033[36m- Running Ar40 imp. submission script --------------------------------\033[0m"
echo

# echo "\033[36m- Reverting to GEMC 5.10 ----------------------------------------------\033[0m"
# module unload gemc
# module load gemc/5.10
# echo

echo "\033[36m- Changing GEMC directory ---------------------------------------------\033[0m"
unsetenv GEMC_DATA_DIR
setenv GEMC_DATA_DIR /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12Tags
# setenv GEMC_DATA_DIR /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12Tags/GEMC_5_11
# # setenv GEMC_DATA_DIR /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12Tags/GEMC_5_10
echo "\033[36mGEMC_DATA_DIR: $GEMC_DATA_DIR\033[0m"
echo

echo "\033[36m- Updating target files -----------------------------------------------\033[0m"
echo

echo "\033[36mUpdating clas12Tags repository...\033[0m"
# echo "\033[36mUpdating detectors repository...\033[0m"
cd /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12Tags
# cd /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/detectors
git pull
git clean -f
# ./targets.pl config.dat
echo

# echo "\033[36mCopying new target files to GEMC_DATA_DIR...\033[0m"
# cd /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/clas12Tags/GEMC_5_10/experiments/clas12/targets
# cp /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/detectors/clas12/targets/target__geometry_RGM_lAr.txt ./
# cp /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/detectors/clas12/targets/target__geometry_RGM_2_C_v2_S.txt ./
# cp /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/detectors/clas12/targets/target__geometry_RGM_2_C_v2_L.txt ./
cd /u/home/asportes/clas12simulations/simulationJ/Simulation/submit/my_submissions/Ar40_imp_tests
echo

echo "\033[36m- Clearing old run files and submitting scripts -----------------------\033[0m"
echo

echo "\033[36mClearing old RGM_2_C run files and submitting scripts...\033[0m"
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_RGM_2_C_test/mchipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_RGM_2_C_test/mchipo/*
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_RGM_2_C_test/reconhipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_RGM_2_C_test/reconhipo/*
sbatch submit_GEMC_RGM_2_C_TEST.sh
echo

echo "\033[36mClearing old RGM_lAr run files and submitting scripts...\033[0m"
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19_5_11_test/mchipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19_5_11_test/mchipo/*
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19_5_11_test/reconhipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19_5_11_test/reconhipo/*
sbatch submit_GEMC_RGM_lAr_TEST.sh
# sbatch submit_GEMC_RGM_lAr.sh
echo
# echo "\033[36mClearing old RGM_lAr run files and submitting scripts...\033[0m"
# ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19/mchipo
# rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19/mchipo/*
# ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19/reconhipo
# rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/Ar40/G18_10a_00_000/4029MeV/Q2_0_19/reconhipo/*
# sbatch submit_GEMC_RGM_lAr.sh
# echo

echo "\033[36mClearing old RGM_2_C_v2_S run files and submitting scripts...\033[0m"
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11_test/mchipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11_test/mchipo/*
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11_test/reconhipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11_test/reconhipo/*
sbatch submit_GEMC_RGM_2_C_v2_S_TEST.sh
# sbatch submit_GEMC_RGM_2_C_v2_S.sh
echo
# echo "\033[36mClearing old RGM_2_C_v2_S run files and submitting scripts...\033[0m"
# ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11/mchipo
# rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11/mchipo/*
# ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11/reconhipo
# rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S_5_11/reconhipo/*
# sbatch submit_GEMC_RGM_2_C_v2_S.sh
# echo
# # echo "\033[36mClearing old RGM_2_C_v2_S run files and submitting scripts...\033[0m"
# # ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S/mchipo
# # rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S/mchipo/*
# # ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S/reconhipo
# # rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_S/reconhipo/*
# # sbatch submit_GEMC_RGM_2_C_v2_S.sh
# # echo

echo "\033[36mClearing old RGM_2_C_v2_L run files and submitting scripts...\033[0m"
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11_test/mchipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11_test/mchipo/*
ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11_test/reconhipo
rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11_test/reconhipo/*
sbatch submit_GEMC_RGM_2_C_v2_L_TEST.sh
# sbatch submit_GEMC_RGM_2_C_v2_L.sh
echo
# echo "\033[36mClearing old RGM_2_C_v2_L run files and submitting scripts...\033[0m"
# ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11/mchipo
# rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11/mchipo/*
# ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11/reconhipo
# rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L_5_11/reconhipo/*
# sbatch submit_GEMC_RGM_2_C_v2_L.sh
# echo
# # echo "\033[36mClearing old RGM_2_C_v2_L run files and submitting scripts...\033[0m"
# # ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L/mchipo
# # rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L/mchipo/*
# # ls /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L/reconhipo
# # rm -rf /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco/2N_Analysis_Reco_Samples/C12/G18_10a_00_000/4029MeV/def_Q2_th_L/reconhipo/*
# # sbatch submit_GEMC_RGM_2_C_v2_L.sh
# # echo
