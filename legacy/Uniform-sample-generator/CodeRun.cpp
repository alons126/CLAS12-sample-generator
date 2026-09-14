#include "Uniform_sample_generator.C"
#include "Uniform_sample_generator_e_tester.C"

#define independent_tester false

/* root CodeRun.cpp -q -b */
/* LUND format: https://gemc.jlab.org/gemc/html/documentation/generator/lund.html */

void CodeRun() {
    std::string baseDir = "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_samples/";
    // std::string baseDir = "/Volumes/Extreme_Pro/University/E4nu/Generated_samples/Uniform_samples/";

    // ========================================================================================
    // Tester_e samples
    // ========================================================================================
    // Uniform_sample_generator_e_tester(2.07052, baseDir + "2070MeV/OutPut_Tester_e/");
    // Uniform_sample_generator_e_tester(4.02962, baseDir + "4029MeV/OutPut_Tester_e/");
    // Uniform_sample_generator_e_tester(5.98636, baseDir + "5986MeV/OutPut_Tester_e/");

    // ========================================================================================
    // Uniform 2070MeV samples
    // ========================================================================================

    // Uniform (e,e') samples
    // ========================================================================================
    // Uniform_sample_generator(true, false, false, 2.07052, baseDir + "2070MeV/OutPut/");
    Uniform_sample_generator(true, false, false, 2.07052, baseDir + "2070MeV_devGEMC_rgm_fall2021_Ar/OutPut/", 4000, 25000);
    // Uniform_sample_generator(true, false, false, 2.07052, baseDir + "2070MeV_devGEMC_rgm_fall2021_C_S/OutPut/");
    // Uniform_sample_generator(true, false, false, 2.07052, baseDir + "2070MeV_devGEMC_rgm_fall2021_Ar/OutPut/");

    // Uniform (e,e'p) samples
    // ========================================================================================
    // Uniform_sample_generator(false, true, false, 2.07052, baseDir + "2070MeV/OutPut/");

    // Uniform (e,e'n) samples
    // ========================================================================================
    // Uniform_sample_generator(false, false, true, 2.07052, baseDir + "2070MeV/OutPut/");
    // Uniform_sample_generator(false, false, true, 2.07052, baseDir + "2070MeV_devGEMC_rgm_fall2021_Ar/OutPut/");
    // Uniform_sample_generator(false, false, true, 2.07052, baseDir + "2070MeV_ConstPn/OutPut/");
    // Uniform_sample_generator(false, false, true, 2.07052, baseDir + "2070MeV_ConstPn_devGEMC_rgm_fall2021_Ar/OutPut/");
    // Uniform_sample_generator(false, false, true, 2.07052, baseDir + "2070MeV_target_zpos_test/OutPut/");

    // ========================================================================================
    // Uniform 4029MeV samples
    // ========================================================================================

    // Uniform (e,e') samples
    // ========================================================================================
    // Uniform_sample_generator(true, false, false, 4.02962, baseDir + "4029MeV/OutPut/");
    // Uniform_sample_generator(true, false, false, 4.02962, baseDir + "4029MeV_devGEMC_rgm_fall2021_C_S/OutPut/");
    // Uniform_sample_generator(true, false, false, 4.02962, baseDir + "4029MeV_devGEMC_rgm_fall2021_Ar/OutPut/");

    // Uniform (e,e'p) samples
    // ========================================================================================
    // Uniform_sample_generator(false, true, false, 4.02962, baseDir + "4029MeV/OutPut/");

    // Uniform (e,e'n) samples
    // ========================================================================================
    // Uniform_sample_generator(false, false, true, 4.02962, baseDir + "4029MeV/OutPut/");
    // Uniform_sample_generator(false, false, true, 4.02962,
    // "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/4029MeV_devGEMC_rgm_fall2021_Ar/OutPut/"); Uniform_sample_generator(false, false,
    // true, 4.02962, baseDir + "4029MeV_ConstPn/OutPut/"); Uniform_sample_generator(false, false,
    // true, 4.02962, baseDir + "4029MeV_ConstPn_devGEMC_rgm_fall2021_Ar/OutPut/");
    // Uniform_sample_generator(false, false, true, 4.02962,
    // "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/4029MeV_target_zpos_test/OutPut/");

    // ========================================================================================
    // Uniform 5986MeV samples
    // ========================================================================================

    // Uniform (e,e') samples
    // ========================================================================================
    // Uniform_sample_generator(true, false, false, 5.98636, baseDir + "5986MeV/OutPut/");
    // Uniform_sample_generator(true, false, false, 5.98636,
    // "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/5986MeV_devGEMC_rgm_fall2021_C_S/OutPut/"); Uniform_sample_generator(true, false,
    // false, 5.98636, baseDir + "5986MeV_devGEMC_rgm_fall2021_Ar/OutPut/");

    // Uniform (e,e'p) samples
    // ========================================================================================
    // Uniform_sample_generator(false, true, false, 5.98636, baseDir + "5986MeV/OutPut/");

    // Uniform (e,e'n) samples
    // ========================================================================================
    // Uniform_sample_generator(false, false, true, 5.98636, baseDir + "5986MeV/OutPut/");
    // Uniform_sample_generator(false, false, true, 5.98636,
    // "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/5986MeV_devGEMC_rgm_fall2021_Ar/OutPut/"); Uniform_sample_generator(false, false,
    // true, 5.98636, baseDir + "5986MeV_ConstPn/OutPut/"); Uniform_sample_generator(false, false,
    // true, 5.98636, baseDir + "5986MeV_ConstPn_devGEMC_rgm_fall2021_Ar/OutPut/");
    // Uniform_sample_generator(false, false, true, 5.98636,
    // "/lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_e-p-n_samples/5986MeV_target_zpos_test/OutPut/");

    // Uniform_sample_generator(false, false, true, 10.6, baseDir + "10600MeV_test/OutPut/");
}
