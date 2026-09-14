//
// Created by Alon Sportes on 22/06/2023.
//

#ifndef AMAPS_H
#define AMAPS_H

#include "AMaps.h"

#include <algorithm>

// AMaps constructors ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* AMaps generation constructor (G1) */
AMaps::AMaps(const std::string& SampleName, const std::string& P_e_bin_profile, const std::string& P_nuc_bin_profile, double beamE, const std::string& AMapsMode, const std::string& SavePath,
             int nOfNucMomBins, int nOfElecMomBins, int hnsNumOfXBins, int hnsNumOfYBins, int hesNumOfXBins, int hesNumOfYBins, const std::string& ElectronLabel,
             const std::string& ProtonLabel, const std::string& NeutronLabel, ParticleType type = UNDEFINED_TYPE)
    : particleType(type) {
    eLabel = ElectronLabel, pLabel = ProtonLabel, nLabel = NeutronLabel;
    Maps_Mode = AMapsMode;
    SName = SampleName;
    AMapSavePath = SavePath;
    HistNucSliceNumOfXBins = hnsNumOfXBins;
    HistNucSliceNumOfYBins = hnsNumOfYBins;
    HistElectronSliceNumOfXBins = hesNumOfXBins;
    HistElectronSliceNumOfYBins = hesNumOfYBins;
    NumberNucOfMomSlices = nOfNucMomBins;
    NumberElecOfMomSlices = nOfElecMomBins;

    const bool hasElectronSample = bt::FindSubstring(SName, "Uniform_1e_sample_");
    const bool hasProtonSample = bt::FindSubstring(SName, "Uniform_ep_sample_");
    const bool hasNeutronSample = bt::FindSubstring(SName, "Uniform_en_sample_");

#pragma region /* Setting saving directories */
    auto EnsureDirectory = [](const std::string& path) { system(("mkdir -p " + path).c_str()); };

    std::string SavePathAMapsBC = AMapSavePath + "00b_AMaps_BC_from_class/";
    EnsureDirectory(SavePathAMapsBC);

    std::string AMapSavePathTLElectron, AMapSavePathRecoElectron, AMapSavePathRecoToTLElectron, AMapSavePathSepAMapsElectron;

    if (hasElectronSample) {
        AMapSavePathTLElectron = SavePath + "01a_Electron_TL_Hit_Maps/";
        EnsureDirectory(AMapSavePathTLElectron);
        AMapSavePathRecoElectron = SavePath + "01b_Electron_Reco_Hit_Maps/";
        EnsureDirectory(AMapSavePathRecoElectron);
        AMapSavePathRecoToTLElectron = SavePath + "01c_Electron_RecoToTL_Ratio/";
        EnsureDirectory(AMapSavePathRecoToTLElectron);
        AMapSavePathSepAMapsElectron = AMapSavePath + "01d_Electron_Separate_AMaps/";
        EnsureDirectory(AMapSavePathSepAMapsElectron);
    }

    std::string AMapSavePathTLProton, AMapSavePathRecoProton, AMapSavePathRecoToTLProton, AMapSavePathSepAMapsProton;

    if (hasProtonSample) {
        AMapSavePathTLProton = SavePath + "02a_Proton_TL_Hit_Maps/";
        EnsureDirectory(AMapSavePathTLProton);
        AMapSavePathRecoProton = SavePath + "02b_Proton_Reco_Hit_Maps/";
        EnsureDirectory(AMapSavePathRecoProton);
        AMapSavePathRecoToTLProton = SavePath + "02c_Proton_RecoToTL_Ratio/";
        EnsureDirectory(AMapSavePathRecoToTLProton);
        AMapSavePathSepAMapsProton = AMapSavePath + "02d_Proton_Separate_AMaps/";
        EnsureDirectory(AMapSavePathSepAMapsProton);
    }

    std::string AMapSavePathTLNeutron, AMapSavePathRecoNeutron, AMapSavePathRecoToTLNeutron, AMapSavePathSepAMapsNeutron;
    std::string AMapConstSavePathTLNeutron, AMapConstSavePathRecoNeutron, AMapConstSavePathRecoToTLNeutron, AMapConstSavePathSepAMapsNeutron;

    if (hasNeutronSample) {
        AMapSavePathTLNeutron = SavePath + "03aa_Neutron_TL_Hit_Maps/";
        EnsureDirectory(AMapSavePathTLNeutron);
        AMapSavePathRecoNeutron = SavePath + "03ab_Neutron_Reco_Hit_Maps/";
        EnsureDirectory(AMapSavePathRecoNeutron);
        AMapSavePathRecoToTLNeutron = SavePath + "03ac_Neutron_RecoToTL_Ratio/";
        EnsureDirectory(AMapSavePathRecoToTLNeutron);
        AMapSavePathSepAMapsNeutron = AMapSavePath + "03ad_Neutron_Separate_AMaps/";
        EnsureDirectory(AMapSavePathSepAMapsNeutron);

        AMapConstSavePathTLNeutron = SavePath + "03ba_Neutron_TL_Hit_Maps_Const/";
        EnsureDirectory(AMapConstSavePathTLNeutron);
        AMapConstSavePathRecoNeutron = SavePath + "03bb_Neutron_Reco_Hit_Maps_Const/";
        EnsureDirectory(AMapConstSavePathRecoNeutron);
        AMapConstSavePathRecoToTLNeutron = SavePath + "03bc_Neutron_RecoToTL_Ratio_Const/";
        EnsureDirectory(AMapConstSavePathRecoToTLNeutron);
        AMapConstSavePathSepAMapsNeutron = AMapSavePath + "03bd_Neutron_Separate_AMaps_Const/";
        EnsureDirectory(AMapConstSavePathSepAMapsNeutron);
    }

    std::string AMapSavePathAMap = AMapSavePath + "04_Finalized_AMaps/";
    EnsureDirectory(AMapSavePathAMap);

    std::string AMapSavePathGeneratedAMapCopy = AMapSavePath + "05_Generated_maps/";
    EnsureDirectory(AMapSavePathGeneratedAMapCopy);
    AMapCopySavePath = AMapSavePathGeneratedAMapCopy;
#pragma endregion

#pragma region /* Setting AMapsMode_TitleAddition */
    std::string AMapsMode_TitleAddition = Maps_Mode.empty() ? "" : Maps_Mode;

    if (!(Maps_Mode == "AMaps" || Maps_Mode == "WMaps")) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Maps_Mode = " + Maps_Mode + " is not defined!"); }

#pragma endregion

    SetBins(P_nuc_bin_profile, beamE);
    SetElectronBins(P_e_bin_profile, beamE);

    auto CreateMap = [&](const std::string& statsTitle, const std::string& title, const std::string& particleLabel, const std::string& savePath, const std::string& saveName, int xBins,
                         int yBins) {
        return hPlot2D(AMapsMode_TitleAddition, "", statsTitle, title, "#phi_{" + particleLabel + "} [#circ]", "#theta_{" + particleLabel + "} [#circ]", savePath, saveName, hBinLowerXLim,
                       hBinUpperXLim, hBinLowerYLim, hBinUpperYLim, xBins, yBins);
    };

    auto CreateAndStoreMap = [&](const std::string& statsTitle, const std::string& title, const std::string& particleLabel, const std::string& savePath, const std::string& saveName,
                                 int xBins, int yBins, std::vector<hPlot2D>& destination, std::vector<TObject*>& rootHistograms) {
        hPlot2D generatedMap = CreateMap(statsTitle, title, particleLabel, savePath, saveName, xBins, yBins);
        destination.push_back(generatedMap);
        rootHistograms.push_back(generatedMap.GetHistogram2D());
    };

#pragma region /* Reco theta VS phi BC */

    if (hasElectronSample) {
        std::string hStatsTitleAMapBCElectron = "Electron_AMap_BC", hTitleAMapBCElectron = "Electron AMap BC", hSaveNameAMapBCElectron = "01_e_AMap_BC";
        reco_theta_e_VS_phi_e_BC =
            CreateMap(hStatsTitleAMapBCElectron, hTitleAMapBCElectron, eLabel, SavePathAMapsBC, hSaveNameAMapBCElectron, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins);
    }

    if (hasProtonSample) {
        std::string hStatsTitleAMapBCProton = "Proton_AMap_BC", hTitleAMapBCProton = "Proton AMap BC", hSaveNameAMapBCProton = "02_p_AMap_BC";
        reco_theta_p_VS_phi_p_BC = CreateMap(hStatsTitleAMapBCProton, hTitleAMapBCProton, pLabel, SavePathAMapsBC, hSaveNameAMapBCProton, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
    }

    if (hasNeutronSample) {
        std::string hStatsTitleAMapBCNeutron = "Neutron_AMap_BC", hTitleAMapBCNeutron = "Neutron AMap BC", hSaveNameAMapBCNeutron = "03_n_AMap_BC";
        reco_theta_n_VS_phi_n_BC = CreateMap(hStatsTitleAMapBCNeutron, hTitleAMapBCNeutron, nLabel, SavePathAMapsBC, hSaveNameAMapBCNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
    }

    // std::string hStatsTitleAMapBCNucleon = "Nucleon_AMap_BC", hTitleAMapBCNucleon = "Nucleon AMap BC", hSaveNameAMapBCNucleon = "04_nuc_AMap_BC";
    // reco_theta_nuc_VS_phi_nuc_BC = hPlot2D(AMapsMode_TitleAddition, "", hStatsTitleAMapBCNucleon, hTitleAMapBCNucleon, "#phi_{nuc} [#circ]", "#theta_{nuc} [#circ]", SavePathAMapsBC,
    //                                        hSaveNameAMapBCNucleon, hBinLowerXLim, hBinUpperXLim, hBinLowerYLim, hBinUpperYLim, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
#pragma endregion

#pragma region /* Setting electron histograms */
    if (hasElectronSample) {
        for (int i = 0; i < ElectronMomSliceLimits.size(); i++) {
            double BinLowerLim = ElectronMomSliceLimits.at(i).at(0), BinUpperLim = ElectronMomSliceLimits.at(i).at(1);

            int BinUpperLimPrecision;
            if (BinUpperLim == beamE) {
                BinUpperLimPrecision = 3;
            } else {
                BinUpperLimPrecision = 2;
            }

            std::string BinDensity = " (" + to_string(HistElectronSliceNumOfXBins) + "x" + to_string(HistElectronSliceNumOfYBins) + ")";

#pragma region /* Setting electron Acceptance maps */

#pragma region /* Electron TL hit map */
            std::string hStatsTitleTLElectron = "TL P_{" + eLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                                bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
            std::string hTitleTLElectron = "TL P_{" + eLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                           bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
            std::string hSaveNameTLElectron =
                to_string(i + 1) + "_TL_P_bin_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
            CreateAndStoreMap(hStatsTitleTLElectron, hTitleTLElectron, eLabel, AMapSavePathTLElectron, hSaveNameTLElectron, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins,
                              truth_theta_e_VS_phi_e_BySlice, truth_e_BySlice);
#pragma endregion

#pragma region /* Electron Reco. Acceptance maps */
            std::string hStatsTitleRecoElectron = "Reco P_{" + eLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                                  bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
            std::string hTitleRecoElectron = "Reco P_{" + eLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                             bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
            std::string hSaveNameRecoElectron =
                to_string(i + 1) + "_Reco_P_bin_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
            CreateAndStoreMap(hStatsTitleRecoElectron, hTitleRecoElectron, eLabel, AMapSavePathRecoElectron, hSaveNameRecoElectron, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins,
                              reco_theta_e_VS_phi_e_BySlice, reco_e_BySlice);
#pragma endregion

#pragma region /* Electron Reco./TL Ratio */
            std::string hStatsTitleRecoToTLRatioElectron = "Electron Reco/TL ratio for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                                           bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
            std::string hTitleRecoToTLRatioElectron = "Electron Reco/TL ratio for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                                      bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
            std::string hSaveNameRecoToTLRatioElectron =
                to_string(i + 1) + "_e_Ratio_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
            CreateAndStoreMap(hStatsTitleRecoToTLRatioElectron, hTitleRecoToTLRatioElectron, eLabel, AMapSavePathRecoToTLElectron, hSaveNameRecoToTLRatioElectron,
                              HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins, acceptance_eff_e_BySlice, acc_eff_e_BySlice);
#pragma endregion

#pragma region /* Electron separate AMaps */
            std::string hStatsTitleSepAMapsElectron = "Electron AMap for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                                      bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c] and (Reco./TL)#geq" +
                                                      bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
            std::string hTitleSepAMapsElectron = "Electron AMap for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + eLabel + "}#leq" +
                                                 bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c] and (Reco./TL)#geq" +
                                                 bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
            std::string hSaveNameSepAMapsElectron =
                to_string(i + 1) + "_e_SepAMap_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
            CreateAndStoreMap(hStatsTitleSepAMapsElectron, hTitleSepAMapsElectron, eLabel, AMapSavePathSepAMapsElectron, hSaveNameSepAMapsElectron, HistElectronSliceNumOfXBins,
                              HistElectronSliceNumOfYBins, filtered_reco_theta_e_VS_phi_e_BySlice, filtered_reco_e_BySlice);
#pragma endregion

#pragma endregion
        }  // End of loop over slices of electron momentum
    }
#pragma endregion

#pragma region /* Setting nucleon histograms */

#pragma region /* Setting nucleon slice histograms */
    if (hasProtonSample || hasNeutronSample) {
        for (int i = 0; i < NucleonMomSliceLimits.size(); i++) {
            double BinLowerLim = NucleonMomSliceLimits.at(i).at(0), BinUpperLim = NucleonMomSliceLimits.at(i).at(1);

            int BinUpperLimPrecision;
            if (BinUpperLim == beamE) {
                BinUpperLimPrecision = 3;
            } else {
                BinUpperLimPrecision = 2;
            }

            std::string BinDensity = " (" + to_string(HistNucSliceNumOfXBins) + "x" + to_string(HistNucSliceNumOfYBins) + ")";

#pragma region /* Setting proton Acceptance maps */
            if (hasProtonSample) {
#pragma region /* Proton TL Acceptance maps */
                std::string hStatsTitleTLProton = "TL P_{" + pLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                                  bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
                std::string hTitleTLProton = "TL P_{" + pLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                             bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
                std::string hSaveNameTLProton =
                    to_string(i + 1) + "_TL_P_bin_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleTLProton, hTitleTLProton, pLabel, AMapSavePathTLProton, hSaveNameTLProton, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins,
                                  truth_theta_p_VS_phi_p_BySlice, truth_p_BySlice);
#pragma endregion

#pragma region /* Proton Reco. Acceptance maps */
                std::string hStatsTitleRecoProton = "Reco P_{" + pLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                                    bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
                std::string hTitleRecoProton = "Reco P_{" + pLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                               bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
                std::string hSaveNameRecoProton =
                    to_string(i + 1) + "_Reco_P_bin_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleRecoProton, hTitleRecoProton, pLabel, AMapSavePathRecoProton, hSaveNameRecoProton, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins,
                                  reco_theta_p_VS_phi_p_BySlice, reco_p_BySlice);
#pragma endregion

#pragma region /* Proton Reco./TL Ratio */
                std::string hStatsTitleRecoToTLRatioProton = "Proton Reco/TL ratio for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                                             bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
                std::string hTitleRecoToTLRatioProton = "Proton Reco/TL ratio for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                                        bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
                std::string hSaveNameRecoToTLRatioProton =
                    to_string(i + 1) + "_p_Ratio_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleRecoToTLRatioProton, hTitleRecoToTLRatioProton, pLabel, AMapSavePathRecoToTLProton, hSaveNameRecoToTLRatioProton, HistNucSliceNumOfXBins,
                                  HistNucSliceNumOfYBins, acceptance_eff_p_BySlice, acc_eff_p_BySlice);
#pragma endregion

#pragma region /* Proton separate AMaps */
                std::string hStatsTitleSepAMapsProton = "Proton AMap for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                                        bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c] and (Reco./TL)#geq" +
                                                        bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
                std::string hTitleSepAMapsProton = "Proton AMap for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + pLabel + "}#leq" +
                                                   bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c] and (Reco./TL)#geq" +
                                                   bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
                std::string hSaveNameSepAMapsProton =
                    to_string(i + 1) + "_p_SepAMap_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleSepAMapsProton, hTitleSepAMapsProton, pLabel, AMapSavePathSepAMapsProton, hSaveNameSepAMapsProton, HistNucSliceNumOfXBins,
                                  HistNucSliceNumOfYBins, filtered_reco_theta_p_VS_phi_p_BySlice, filtered_reco_p_BySlice);
#pragma endregion
            }
#pragma endregion

#pragma region /* Setting neutron Acceptance maps */
            if (hasNeutronSample) {
#pragma region /* Neutron TL Acceptance maps */
                std::string hStatsTitleTLNeutron = "TL P_{" + nLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                   bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
                std::string hTitleTLNeutron = "TL P_{" + nLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                              bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
                std::string hSaveNameTLNeutron =
                    to_string(i + 1) + "_TL_P_bin_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleTLNeutron, hTitleTLNeutron, nLabel, AMapSavePathTLNeutron, hSaveNameTLNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins,
                                  truth_theta_n_VS_phi_n_BySlice, truth_n_BySlice);
#pragma endregion

#pragma region /* Neutron TL Acceptance maps - const */
                CreateAndStoreMap(hStatsTitleTLNeutron + " const", hTitleTLNeutron, nLabel, AMapConstSavePathTLNeutron, hSaveNameTLNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins,
                                  truth_theta_n_VS_phi_n_ByConstSlice, truth_n_ByConstSlice);
#pragma endregion

#pragma region /* Neutron Reco. Acceptance maps */
                std::string hStatsTitleRecoNeutron = "Reco P_{" + nLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                     bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
                std::string hTitleRecoNeutron = "Reco P_{" + nLabel + "} bin for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
                std::string hSaveNameRecoNeutron =
                    to_string(i + 1) + "_Reco_P_bin_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleRecoNeutron, hTitleRecoNeutron, nLabel, AMapSavePathRecoNeutron, hSaveNameRecoNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins,
                                  reco_theta_n_VS_phi_n_BySlice, reco_n_BySlice);
#pragma endregion

#pragma region /* Neutron Reco. Acceptance maps - const */
                CreateAndStoreMap(hStatsTitleRecoNeutron + " const", hTitleRecoNeutron, nLabel, AMapConstSavePathRecoNeutron, hSaveNameRecoNeutron, HistNucSliceNumOfXBins,
                                  HistNucSliceNumOfYBins, reco_theta_n_VS_phi_n_ByConstSlice, reco_n_ByConstSlice);
#pragma endregion

#pragma region /* Neutron Reco./TL Ratio */
                std::string hStatsTitleRecoToTLRatioNeutron = "Neutron Reco/TL ratio for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                              bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]";
                std::string hTitleRecoToTLRatioNeutron = "Neutron Reco/TL ratio for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                         bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c]" + BinDensity;
                std::string hSaveNameRecoToTLRatioNeutron =
                    to_string(i + 1) + "_p_Ratio_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleRecoToTLRatioNeutron, hTitleRecoToTLRatioNeutron, nLabel, AMapSavePathRecoToTLNeutron, hSaveNameRecoToTLRatioNeutron, HistNucSliceNumOfXBins,
                                  HistNucSliceNumOfYBins, acceptance_eff_n_BySlice, acc_eff_n_BySlice);
#pragma endregion

#pragma region /* Neutron Reco./TL Ratio - const */
                CreateAndStoreMap(hStatsTitleRecoToTLRatioNeutron + " const", hTitleRecoToTLRatioNeutron, nLabel, AMapConstSavePathRecoToTLNeutron, hSaveNameRecoToTLRatioNeutron,
                                  HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, acceptance_eff_n_ByConstSlice, acc_eff_n_ByConstSlice);
#pragma endregion

#pragma region /* Neutron separate AMaps */
                std::string hStatsTitleSepAMapsNeutron = "Neutron AMap for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                         bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c] and (Reco./TL)#geq" +
                                                         bt::ToStringWithPrecision(Neutral_particle_min_Ratio, 2);
                std::string hTitleSepAMapsNeutron = "Neutron AMap for " + bt::ToStringWithPrecision(BinLowerLim, 2) + "#leqP^{truth}_{" + nLabel + "}#leq" +
                                                    bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision) + " [GeV/c] and (Reco./TL)#geq" +
                                                    bt::ToStringWithPrecision(Neutral_particle_min_Ratio, 2);
                std::string hSaveNameSepAMapsNeutron =
                    to_string(i + 1) + "_n_SepAMap_for_P_from_" + bt::ToStringWithPrecision(BinLowerLim, 2) + "_to_" + bt::ToStringWithPrecision(BinUpperLim, BinUpperLimPrecision);
                CreateAndStoreMap(hStatsTitleSepAMapsNeutron, hTitleSepAMapsNeutron, nLabel, AMapSavePathSepAMapsNeutron, hSaveNameSepAMapsNeutron, HistNucSliceNumOfXBins,
                                  HistNucSliceNumOfYBins, filtered_reco_theta_n_VS_phi_n_BySlice, filtered_reco_n_BySlice);
#pragma endregion

#pragma region /* Neutron separate AMaps - const */
                CreateAndStoreMap(hStatsTitleSepAMapsNeutron + " const", hTitleSepAMapsNeutron, nLabel, AMapConstSavePathSepAMapsNeutron, hSaveNameSepAMapsNeutron, HistNucSliceNumOfXBins,
                                  HistNucSliceNumOfYBins, filtered_reco_theta_n_VS_phi_n_ByConstSlice, filtered_reco_n_ByConstSlice);
#pragma endregion
            }
#pragma endregion
        }  // End of loop over slices of nucleon momentum
    }
#pragma endregion

#pragma region /* Setting neutron Acceptance maps */
    if (hasNeutronSample) {
#pragma region /* Neutron TL Acceptance maps */
        std::string hStatsTitleTLNeutron = "TL Neutron Hit Map", hTitleTLNeutron = "TL Neutron Hit Map", hSaveNameTLNeutron = "TL_n_AMap";
        truth_theta_n_VS_phi_n = CreateMap(hStatsTitleTLNeutron, hTitleTLNeutron, nLabel, AMapSavePathTLNeutron, hSaveNameTLNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
#pragma endregion

#pragma region /* Neutron Reco. Acceptance maps */
        std::string hStatsTitleRecoNeutron = "Reco Neutron Hit Map", hTitleRecoNeutron = "Reco Neutron Hit Map", hSaveNameRecoNeutron = "Reco_n_AMap";
        reco_theta_n_VS_phi_n = CreateMap(hStatsTitleRecoNeutron, hTitleRecoNeutron, nLabel, AMapSavePathRecoNeutron, hSaveNameRecoNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
#pragma endregion

#pragma region /* Neutron Reco./TL Ratio */
        std::string hStatsTitleRecoToTLNeutron = "Neutron Reco/TL ratio", hTitleRecoToTLNeutron = "Neutron Reco/TL ratio", hSaveNameRecoToTLNeutron = "Neutron_Ratio";
        acceptance_eff_n =
            CreateMap(hStatsTitleRecoToTLNeutron, hTitleRecoToTLNeutron, nLabel, AMapSavePathRecoToTLNeutron, hSaveNameRecoToTLNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
#pragma endregion
    }
#pragma endregion

#pragma endregion

#pragma region /* Finalized acceptance maps */
    if (hasElectronSample) {
        std::string hStatsTitleAMapElectron = "Electron_AMap";
        std::string hTitleAMapElectron = "Electron AMap for (Reco./TL)#geq" + bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
        std::string hSaveNameAMapElectron = "01_e_AMap";
        filtered_reco_theta_e_VS_phi_e =
            CreateMap(hStatsTitleAMapElectron, hTitleAMapElectron, eLabel, AMapSavePathAMap, hSaveNameAMapElectron, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins);
    }

    if (hasProtonSample) {
        std::string hStatsTitleAMapProton = "Proton_AMap";
        std::string hTitleAMapProton = "Proton AMap for (Reco./TL)#geq" + bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
        std::string hSaveNameAMapProton = "02_p_AMap";
        filtered_reco_theta_p_VS_phi_p = CreateMap(hStatsTitleAMapProton, hTitleAMapProton, pLabel, AMapSavePathAMap, hSaveNameAMapProton, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
    }

    if (hasNeutronSample) {
        std::string hStatsTitleAMapNeutron = "Neutron_AMap";
        std::string hTitleAMapNeutron = "Neutron AMap for (Reco./TL)#geq" + bt::ToStringWithPrecision(Neutral_particle_min_Ratio, 2);
        std::string hSaveNameAMapNeutron = "03_n_AMap";
        filtered_reco_theta_n_VS_phi_n = CreateMap(hStatsTitleAMapNeutron, hTitleAMapNeutron, nLabel, AMapSavePathAMap, hSaveNameAMapNeutron, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
    }

    // std::string hStatsTitleAMapNucleon = "Nucleon_AMap";
    // std::string hTitleAMapNucleon = "Nucleon AMap for (Reco./TL)_{"+nLabel+"}#geq" + bt::ToStringWithPrecision(Neutral_particle_min_Ratio, 2) + " and (Reco./TL)_{c}#geq" +
    //                                 bt::ToStringWithPrecision(Charged_particle_min_Ratio, 2);
    // std::string hSaveNameAMapNucleon = "04_nuc_AMap";
    // filtered_reco_theta_nuc_VS_phi_nuc = hPlot2D(AMapsMode_TitleAddition, "", hStatsTitleAMapNucleon, hTitleAMapNucleon, "#phi_{nuc} [#circ]", "#theta_{nuc} [#circ]", AMapSavePathAMap,
    //                                              hSaveNameAMapNucleon, hBinLowerXLim, hBinUpperXLim, hBinLowerYLim, hBinUpperYLim, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins);
#pragma endregion
}
#pragma endregion

#pragma region /* AMaps loading constructor */
AMaps::AMaps(const std::string& AcceptanceMapsDirectory, const double& beamE, const std::string& AMapsMode, const bool& Electron_single_slice_test, const bool& Nucleon_single_slice_test,
             const std::vector<int>& TestSlices, ParticleType type = UNDEFINED_TYPE)
    : particleType(type) {
    bool PrintOut = true;

    Maps_Mode = AMapsMode;

    std::cout << "\n\nSetting uniform sample names...\n";
    std::string BeamE_str = bt::GetBeamEnergyFromDouble(beamE);
    std::string Electron_source_folder = "Uniform_1e_sample_" + BeamE_str;
    std::string Proton_source_folder = "Uniform_ep_sample_" + BeamE_str;
    std::string Neutron_source_folder = "Uniform_en_sample_" + BeamE_str;

    std::cout << "\n\nLoading slices and their limits...\n";
    /* Load slices and their limits */
    const auto readSliceLimitsWithFallback = [&](const std::string& sampleFolder, const std::string& particleTag, std::vector<std::vector<double>>& destination) {
        const std::string basePath = AcceptanceMapsDirectory + sampleFolder + "/";
        std::vector<std::string> candidatePaths;

        if (Maps_Mode == "AMaps") {
            candidatePaths.push_back(basePath + particleTag + "_AMaps_by_slice/" + particleTag + "_slice_limits.par");
            candidatePaths.push_back(basePath + particleTag + "_AMap_by_slice/" + particleTag + "_slice_limits.par");
        } else if (Maps_Mode == "WMaps") {
            candidatePaths.push_back(basePath + particleTag + "_WMaps_by_slice/" + particleTag + "_slice_limits.par");
            candidatePaths.push_back(basePath + particleTag + "_WMap_by_slice/" + particleTag + "_slice_limits.par");
        }

        for (const auto& path : candidatePaths) {
            if (!bt::fileExists(path.c_str())) { continue; }
            ReadAMapLimits(path.c_str(), destination);
            return;
        }

        if (debugging::AMapsDebuggerMode) {
            std::cout << "\n\033[33mAMaps::AMaps warning: could not find slice limits for particle tag '" << particleTag << "' in:\033[0m\n";
            for (const auto& path : candidatePaths) { std::cout << "  - " << path << "\n"; }
        }
    };

    readSliceLimitsWithFallback(Electron_source_folder, "e", Loaded_ElectronMomSliceLimits);
    readSliceLimitsWithFallback(Proton_source_folder, "p", Loaded_NucleonMomSliceLimits);
    if (Loaded_NucleonMomSliceLimits.empty()) {
        // Some runs provide nucleon limits only under the neutron sample directory.
        readSliceLimitsWithFallback(Neutron_source_folder, "n", Loaded_NucleonMomSliceLimits);
    }

    // Safety check - debugging outputs for loaded slice limits (electrons):
    if (Loaded_ElectronMomSliceLimits.size() == 0) {
        bt::PrintWarning(__func__, __FILE__, __LINE__, "no electron slice limits were loaded from file!");
    } else if (debugging::AMapsDebuggerMode) {
        std::cout << "\n\n\033[35mLoaded Electron Slice Limits (" << Maps_Mode << "):\n\033[0m";
        for (const auto& limit_pair : Loaded_ElectronMomSliceLimits) { std::cout << "Lower: " << limit_pair.at(0) << " , Upper: " << limit_pair.at(1) << "\n"; }
    }

    // Safety check - debugging outputs for loaded slice limits (electrons):
    if (Loaded_NucleonMomSliceLimits.size() == 0) {
        bt::PrintWarning(__func__, __FILE__, __LINE__, "no nucleon slice limits were loaded from file!");
    } else if (debugging::AMapsDebuggerMode) {
        std::cout << "\n\n\033[35mLoaded Nucleon Slice Limits (" << Maps_Mode << "):\n\033[0m";
        for (const auto& limit_pair : Loaded_NucleonMomSliceLimits) { std::cout << "Lower: " << limit_pair.at(0) << " , Upper: " << limit_pair.at(1) << "\n"; }
    }

    std::cout << "\n\nLoad separate maps\n";
    /* Load separate maps */
    if (Maps_Mode == "AMaps") {
        if (particleType == ELECTRON_TYPE) {
            ReadMapSlices(Electron_source_folder, AcceptanceMapsDirectory, "Electron", Loaded_ElectronMomSliceLimits, &Loaded_e_AMap_Slices);
            ReadMapSlices(Electron_source_folder, AcceptanceMapsDirectory, "Electron", Loaded_ElectronMomSliceLimits, &Loaded_e_AMap_Slices_extended, nullptr, true);
        } else if (particleType == PROTON_TYPE) {
            ReadMapSlices(Proton_source_folder, AcceptanceMapsDirectory, "Proton", Loaded_NucleonMomSliceLimits, &Loaded_p_AMap_Slices);
        } else if (particleType == NEUTRON_TYPE) {
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, &Loaded_n_AMap_Slices);
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, &Loaded_n_AMap_ConstSlices, nullptr, true);
        } else {
            // Load everything if particleType is UNDEFINED_TYPE
            // TODO: rethink this case!
            ReadMapSlices(Electron_source_folder, AcceptanceMapsDirectory, "Electron", Loaded_ElectronMomSliceLimits, &Loaded_e_AMap_Slices);
            ReadMapSlices(Electron_source_folder, AcceptanceMapsDirectory, "Electron", Loaded_ElectronMomSliceLimits, &Loaded_e_AMap_Slices_extended, nullptr, true);
            ReadMapSlices(Proton_source_folder, AcceptanceMapsDirectory, "Proton", Loaded_NucleonMomSliceLimits, &Loaded_p_AMap_Slices);
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, &Loaded_n_AMap_Slices);
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, &Loaded_n_AMap_ConstSlices, nullptr, true);
        }
    } else {
        if (particleType == ELECTRON_TYPE) {
            ReadMapSlices(Electron_source_folder, AcceptanceMapsDirectory, "Electron", Loaded_ElectronMomSliceLimits, nullptr, &Loaded_e_WMap_Slices);
        } else if (particleType == PROTON_TYPE) {
            ReadMapSlices(Proton_source_folder, AcceptanceMapsDirectory, "Proton", Loaded_NucleonMomSliceLimits, nullptr, &Loaded_p_WMap_Slices);
        } else if (particleType == NEUTRON_TYPE) {
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, nullptr, &Loaded_n_WMap_Slices);
        } else {
            // Load everything if particleType is UNDEFINED_TYPE
            // TODO: rethink this case!
            ReadMapSlices(Electron_source_folder, AcceptanceMapsDirectory, "Electron", Loaded_ElectronMomSliceLimits, nullptr, &Loaded_e_WMap_Slices);
            ReadMapSlices(Proton_source_folder, AcceptanceMapsDirectory, "Proton", Loaded_NucleonMomSliceLimits, nullptr, &Loaded_p_WMap_Slices);
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, nullptr, &Loaded_n_WMap_Slices);
            ReadMapSlices(Neutron_source_folder, AcceptanceMapsDirectory, "Neutron", Loaded_NucleonMomSliceLimits, nullptr, &Loaded_n_WMap_ConstSlices, true);
        }
    }

    if (PrintOut) {
        if (Maps_Mode == "AMaps") {
            // Safety check - debugging outputs for loaded maps (electrons):
            if (Loaded_e_AMap_Slices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no electron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded electron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_e_AMap_Slices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded extended maps (electrons):
            if (Loaded_e_AMap_Slices_extended.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no extended electron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded extended electron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_e_AMap_Slices_extended) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded maps (protons):
            if (Loaded_p_AMap_Slices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no proton maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded proton map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_p_AMap_Slices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded maps (neutron):
            if (Loaded_n_AMap_Slices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no neutron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded neutron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_n_AMap_Slices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded ConstSlices maps (neutron):
            if (Loaded_n_AMap_ConstSlices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no ConstSlices neutron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded ConstSlices neutron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_n_AMap_ConstSlices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }
        } else if (Maps_Mode == "WMaps") {
            // Safety check - debugging outputs for loaded maps (electrons):
            if (Loaded_e_WMap_Slices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no electron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded electron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_e_WMap_Slices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded maps (protons):
            if (Loaded_p_WMap_Slices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no proton maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded proton map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_p_WMap_Slices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded maps (neutron):
            if (Loaded_n_WMap_Slices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no neutron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded neutron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_n_WMap_Slices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }

            // Safety check - debugging outputs for loaded ConstSlices maps (neutron):
            if (Loaded_n_WMap_ConstSlices.size() == 0) {
                bt::PrintWarning(__func__, __FILE__, __LINE__, "no ConstSlices neutron maps were loaded from file!");
            } else if (debugging::AMapsDebuggerMode) {
                std::cout << "\n\n\033[35mLoaded ConstSlices neutron map size (" << Maps_Mode << "):\n\033[0m";
                for (const auto& slice : Loaded_n_WMap_ConstSlices) { std::cout << "BinX: " << slice.at(0).size() << " , BinY: " << slice.size() << "\n"; }
            }
        }
    }

    if (Maps_Mode == "AMaps" && !Loaded_e_AMap_Slices.empty() && !Loaded_e_AMap_Slices.at(0).empty() && !Loaded_e_AMap_Slices.at(0).at(0).empty()) {
        HistElectronSliceNumOfXBins = Loaded_e_AMap_Slices.at(0).at(0).size();
        HistElectronSliceNumOfYBins = Loaded_e_AMap_Slices.at(0).size();
    } else if (Maps_Mode == "WMaps" && !Loaded_e_WMap_Slices.empty() && !Loaded_e_WMap_Slices.at(0).empty() && !Loaded_e_WMap_Slices.at(0).at(0).empty()) {
        HistElectronSliceNumOfXBins = Loaded_e_WMap_Slices.at(0).at(0).size();
        HistElectronSliceNumOfYBins = Loaded_e_WMap_Slices.at(0).size();
    } else {
        bt::PrintWarning(__func__, __FILE__, __LINE__, "electron map slices were not loaded (or are empty). Using defaults.");
        HistElectronSliceNumOfXBins = 100;
        HistElectronSliceNumOfYBins = 100;
    }

    // Nucleon binning: only read nucleon map dimensions if those maps were actually loaded.
    // In electron-only mode, neutron/proton slices are not loaded, and indexing would segfault.
    if (Maps_Mode == "AMaps" && !Loaded_n_AMap_ConstSlices.empty() && !Loaded_n_AMap_ConstSlices.at(0).empty() && !Loaded_n_AMap_ConstSlices.at(0).at(0).empty()) {
        HistNucSliceNumOfXBins = Loaded_n_AMap_ConstSlices.at(0).at(0).size();
        HistNucSliceNumOfYBins = Loaded_n_AMap_ConstSlices.at(0).size();
    } else if (Maps_Mode == "AMaps" && !Loaded_p_AMap_Slices.empty() && !Loaded_p_AMap_Slices.at(0).empty() && !Loaded_p_AMap_Slices.at(0).at(0).empty()) {
        HistNucSliceNumOfXBins = Loaded_p_AMap_Slices.at(0).at(0).size();
        HistNucSliceNumOfYBins = Loaded_p_AMap_Slices.at(0).size();
    } else if (Maps_Mode == "AMaps" && !Loaded_n_AMap_Slices.empty() && !Loaded_n_AMap_Slices.at(0).empty() && !Loaded_n_AMap_Slices.at(0).at(0).empty()) {
        HistNucSliceNumOfXBins = Loaded_n_AMap_Slices.at(0).at(0).size();
        HistNucSliceNumOfYBins = Loaded_n_AMap_Slices.at(0).size();
    } else if (Maps_Mode == "WMaps" && !Loaded_p_WMap_Slices.empty() && !Loaded_p_WMap_Slices.at(0).empty() && !Loaded_p_WMap_Slices.at(0).at(0).empty()) {
        HistNucSliceNumOfXBins = Loaded_p_WMap_Slices.at(0).at(0).size();
        HistNucSliceNumOfYBins = Loaded_p_WMap_Slices.at(0).size();
    } else if (Maps_Mode == "WMaps" && !Loaded_n_WMap_Slices.empty() && !Loaded_n_WMap_Slices.at(0).empty() && !Loaded_n_WMap_Slices.at(0).at(0).empty()) {
        HistNucSliceNumOfXBins = Loaded_n_WMap_Slices.at(0).at(0).size();
        HistNucSliceNumOfYBins = Loaded_n_WMap_Slices.at(0).size();
    } else {
        // Fall back to the historic defaults used throughout the project.
        HistNucSliceNumOfXBins = 75;
        HistNucSliceNumOfYBins = 75;
    }

    e_single_slice_test = Electron_single_slice_test, nuc_single_slice_test = Nucleon_single_slice_test;
    Slices2Test = TestSlices;

    if (particleType == ELECTRON_TYPE && Loaded_ElectronMomSliceLimits.size() == 0) { bt::PrintWarning(__func__, __FILE__, __LINE__, "no electron slice limits were loaded!"); }

    if ((particleType == PROTON_TYPE || particleType == NEUTRON_TYPE) && Loaded_NucleonMomSliceLimits.size() == 0) {
        bt::PrintWarning(__func__, __FILE__, __LINE__, "no nucleon slice limits were loaded!");
    }

    // exit(1);
}
#pragma endregion

// SetBins functions ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* SetBins function (G1a) */
// This function sets the momentum slices for the nucleons (protons and neutrons).
void AMaps::SetBins(const std::string& P_nuc_bin_profile, double beamE) {
    bool InvertedPrintOut = false;
    bool RegPrintOut = false;

    if (P_nuc_bin_profile == "equi_inverted_P_nuc") {
        double InvertedPLowerLim = (1 / beamE);
        double InvertedPUpper = (1 / Nucleon_Momentum_Slice_Th);
        double Delta = (InvertedPUpper - InvertedPLowerLim) / NumberNucOfMomSlices;

        for (int i = 0; i < NumberNucOfMomSlices; i++) {
            double InvertedBinLower = InvertedPLowerLim + i * Delta;
            double InvertedBinUpper = InvertedBinLower + Delta;

            if (InvertedPrintOut) {
                std::cout << "\n\nP_nuc_bin_profile = " << P_nuc_bin_profile << "\n";
                std::cout << "InvertedBinLower = " << InvertedBinLower << "\n";
                std::cout << "InvertedBinUpper = " << InvertedBinUpper << "\n";
                std::cout << "i = " << i << "\n";
                std::cout << "Delta = " << Delta << "\n\n";
            }

            InvertedNucleonMomSliceLimits.push_back({InvertedBinLower, InvertedBinUpper});
        }

        if (InvertedPrintOut && !RegPrintOut) { exit(1); }

        for (int i = (NumberNucOfMomSlices - 1); i >= 0; i--) {
            double BinLower = 1 / InvertedNucleonMomSliceLimits.at(i).at(1);
            double BinUpper = 1 / InvertedNucleonMomSliceLimits.at(i).at(0);

            if (RegPrintOut) {
                std::cout << "\n\nBinLower = " << BinLower << "\n";
                std::cout << "BinUpper = " << BinUpper << "\n";
                std::cout << "i = " << i << "\n";
            }

            NucleonMomSliceLimits.push_back({BinLower, BinUpper});
        }

        if (RegPrintOut) { exit(1); }
    } else if (P_nuc_bin_profile == "varying_P_nuc_bins") {
        // TODO: separate by SampleName?
        bool RegPrintOut = false;

        NucleonMomSliceLimits = CustomNucleonMomSliceLimits_C12x4_simulation_G18_Q204_6GeV;
        NumberNucOfMomSlices = NucleonMomSliceLimits.size();

        if (RegPrintOut) {
            for (int i = 0; i < NucleonMomSliceLimits.size(); i++) {
                std::cout << "\n\nP_nuc_bin_profile = " << P_nuc_bin_profile << "\n";
                std::cout << "SliceLowerLimit = " << NucleonMomSliceLimits.at(i).at(0) << "\n";
                std::cout << "SliceUpperLimit = " << NucleonMomSliceLimits.at(i).at(1) << "\n";
                std::cout << "i = " << i << "\n";
            }

            exit(1);
        }
    } else if (P_nuc_bin_profile == "uniform_P_nuc_bins") {
        double PLowerLim = Nucleon_Momentum_Slice_Th;
        double PUpperLim = beamE;
        double Delta = (PUpperLim - PLowerLim) / NumberNucOfMomSlices;

        int Num_of_bins = 0;  // For monitoring purposes only!

        for (int i = 0; i < NumberNucOfMomSlices; i++) {
            double BinLower = PLowerLim + i * Delta;
            double BinUpper = BinLower + Delta;

            NucleonMomSliceLimits.push_back({BinLower, BinUpper});
            ++Num_of_bins;

            if (RegPrintOut) {
                std::cout << "\n\nP_nuc_bin_profile = " << P_nuc_bin_profile << "\n";
                std::cout << "BinLower = " << BinLower << "\n";
                std::cout << "BinUpper = " << BinUpper << "\n";
                std::cout << "i = " << i << "\n";
                std::cout << "Num_of_bins = " << Num_of_bins << "\n";
                std::cout << "Delta = " << Delta << "\n\n";
            }
        }

        if (RegPrintOut) { exit(1); }
    } else {
        bt::ExitWithError(__func__, __FILE__, __LINE__,
                          "No valid P_nuc_bin_profile selected! Choose between:\n"
                          "equi_inverted_P_nuc , varying_P_nuc_bins , uniform_P_nuc_bins");
    }
}
#pragma endregion

#pragma region /* SetElectronBins function (G1b) */
// This function sets the momentum slices for the electron.
void AMaps::SetElectronBins(const std::string& P_e_bin_profile, double beamE) {
    bool InvertedPrintOut = false;
    bool RegPrintOut = false;

    if (P_e_bin_profile == "reformat_e_bins") {
        double InvertedPLowerLim = (1 / beamE);
        double InvertedPUpperLim = (1 / Nucleon_Momentum_Slice_Th);
        double p2 = 1.57819;
        double Delta = (InvertedPUpperLim - InvertedPLowerLim) / NumberNucOfMomSlices;
        double delta = (InvertedPUpperLim - (1 / p2));
        double Ratio = (InvertedPUpperLim - InvertedPLowerLim) / delta;

        int NumOfElectronMomBins;

        bool SliceAndDice = true;
        int iter = 0;

        while (SliceAndDice) {
            double InvBinLower;
            double InvBinUpper;
            double deltaLoop = delta;

            if (iter == 0) {
                InvBinUpper = InvertedPUpperLim;
                InvBinLower = InvBinUpper - deltaLoop;
            } else {
                InvBinUpper = InvBinLower;

                deltaLoop = delta / 12;

                if (InvBinUpper - deltaLoop > 0) {
                    InvBinLower = InvBinUpper - deltaLoop;
                } else {
                    InvBinLower = InvertedPLowerLim;
                }
            }

            if (InvertedPrintOut) {
                std::cout << "\n\nP_e_bin_profile = " << P_e_bin_profile << "\n";
                std::cout << "InvBinLower = " << InvBinLower << "\n";
                std::cout << "InvBinUpper = " << InvBinUpper << "\n";
                std::cout << "iter = " << iter << "\n";
                std::cout << "delta = " << delta << "\n";
                std::cout << "deltaLoop = " << deltaLoop << "\n\n";
            }

            if (InvBinLower >= InvertedPLowerLim) {
                InvertedElectronMomSliceLimits.push_back({InvBinLower, InvBinUpper});
                ++iter;
            } else {
                SliceAndDice = false;
            }
        }

        NumOfElectronMomBins = InvertedElectronMomSliceLimits.size();

        if (InvertedPrintOut && !RegPrintOut) { exit(1); }

        if (RegPrintOut) { std::cout << "\n\n---------------------------------------------------\n"; }

        for (int i = 0; i < NumOfElectronMomBins; i++) {
            double BinLower = 1 / InvertedElectronMomSliceLimits.at(i).at(1);
            double BinUpper = 1 / InvertedElectronMomSliceLimits.at(i).at(0);

            if (RegPrintOut) {
                std::cout << "\n\nP_e_bin_profile = " << P_e_bin_profile << "\n";
                std::cout << "BinLower = " << BinLower << "\n";
                std::cout << "BinUpper = " << BinUpper << "\n";
                std::cout << "i = " << i << "\n";
            }

            ElectronMomSliceLimits.push_back({BinLower, BinUpper});
        }

        if (RegPrintOut) { exit(1); }
    } else if (P_e_bin_profile == "varying_P_e_bins") {
        if (bt::FindSubstring(SName, "C12_simulation_6GeV_T5")) {  // Old sample
            ElectronMomSliceLimits = CustomElectronMomSliceLimits_C12_simulation_6GeV_T5;
        } else {  // New sample (24M; 1-foil & 4-foil)
            ElectronMomSliceLimits = CustomElectronMomSliceLimits_C12x4_simulation_G18_Q204_6GeV;
        }

        int NumOfElectronMomBins = ElectronMomSliceLimits.size();

        if (RegPrintOut) {
            for (int i = 0; i < NumOfElectronMomBins; i++) {
                std::cout << "\n\nP_e_bin_profile = " << P_e_bin_profile << "\n";
                std::cout << "ElectronMomSliceLimits.at(" << i << ").at(" << 0 << ") = " << ElectronMomSliceLimits.at(i).at(0) << "\n";
                std::cout << "ElectronMomSliceLimits.at(" << i << ").at(" << 1 << ") = " << ElectronMomSliceLimits.at(i).at(1) << "\n";
            }
        }

        if (RegPrintOut) { exit(1); }
    } else if (P_e_bin_profile == "uniform_P_e_bins") {
        double PLowerLim = 0;
        double PUpper = beamE;
        double Delta = (PUpper - PLowerLim) / NumberElecOfMomSlices;

        int Num_of_bins = 0;  // For monitoring purposes only!

        for (int i = 0; i < NumberElecOfMomSlices; i++) {
            double BinLower = PLowerLim + i * Delta;
            double BinUpper = BinLower + Delta;

            ElectronMomSliceLimits.push_back({BinLower, BinUpper});
            ++Num_of_bins;

            if (RegPrintOut) {
                std::cout << "\n\nP_e_bin_profile = " << P_e_bin_profile << "\n";
                std::cout << "BinLower = " << BinLower << "\n";
                std::cout << "BinUpper = " << BinUpper << "\n";
                std::cout << "i = " << i << "\n";
                std::cout << "Num_of_bins = " << Num_of_bins << "\n";
                std::cout << "Delta = " << Delta << "\n\n";
            }
        }

        if (RegPrintOut) { exit(1); }
    } else if (P_e_bin_profile == "equi_inverted_P_e") {
        double InvertedPLowerLim = (1 / beamE);
        double InvertedPUpper = (1 / Electron_Momentum_Slice_Th);
        double Delta = (InvertedPUpper - InvertedPLowerLim) / NumberElecOfMomSlices;

        for (int i = 0; i < (NumberElecOfMomSlices - 1); i++) {
            double InvertedBinLower = InvertedPLowerLim + i * Delta;
            double InvertedBinUpper = InvertedBinLower + Delta;

            if (InvertedPrintOut) {
                std::cout << "\n\nP_e_bin_profile = " << P_e_bin_profile << "\n";
                std::cout << "InvertedBinLower = " << InvertedBinLower << "\n";
                std::cout << "InvertedBinUpper = " << InvertedBinUpper << "\n";
                std::cout << "i = " << i << "\n";
                std::cout << "Delta = " << Delta << "\n\n";
            }

            InvertedElectronMomSliceLimits.push_back({InvertedBinLower, InvertedBinUpper});
        }

        if (InvertedPrintOut && !RegPrintOut) { exit(1); }

        if (RegPrintOut) { std::cout << "\n\n---------------------------------------------\n\n"; }

        for (int i = 0; i < (NumberElecOfMomSlices - 1); i++) {
            double BinLower = beamE - (1 / InvertedElectronMomSliceLimits.at(i).at(0));
            double BinUpper = beamE - (1 / InvertedElectronMomSliceLimits.at(i).at(1));

            ElectronMomSliceLimits.push_back({BinLower, BinUpper});
        }

        ElectronMomSliceLimits.push_back({ElectronMomSliceLimits.at(ElectronMomSliceLimits.size() - 1).at(1), beamE});

        if (RegPrintOut) {
            for (int i = 0; i < ElectronMomSliceLimits.size(); i++) {
                double BinLower = ElectronMomSliceLimits.at(i).at(0);
                double BinUpper = ElectronMomSliceLimits.at(i).at(1);

                std::cout << "\nP_e_bin_profile = " << P_e_bin_profile << "\n";
                std::cout << "BinLower = " << BinLower << "\n";
                std::cout << "BinUpper = " << BinUpper << "\n";
                std::cout << "i = " << i << "\n\n";
            }
        }

        if (RegPrintOut) { exit(1); }
    } else {
        bt::ExitWithError(__func__, __FILE__, __LINE__,
                          "No valid P_e_bin_profile selected! Choose between:\n"
                          "reformat_e_bins , varying_P_e_bins , uniform_P_e_bins, equi_inverted_P_e");
    }
}
#pragma endregion

#pragma region /* SetBins function (old) */
void AMaps::SetBins(double beamE, double nOfMomBins) {
    double BinUpperLim = beamE;

    bool SliceAndDice = true;
    int BinNumber = 1;

    while (SliceAndDice) {
        double UpperLim, LowerLim;

        if (BinNumber == 1) {
            UpperLim = BinUpperLim;
            LowerLim = (BinUpperLim / nOfMomBins);
        } else {
            UpperLim = LowerLim;
            LowerLim = (UpperLim / nOfMomBins);
        }

        NucleonMomSliceLimits.push_back({LowerLim, UpperLim});

        if (LowerLim <= Nucleon_Momentum_Slice_Th) {
            SliceAndDice = false;
        } else {
            ++BinNumber;
        }
    }
}
#pragma endregion

// isElectron function --------------------------------------------------------------------------------------------------------------------------------------------------

bool AMaps::IsOneOf(const std::string& value, std::initializer_list<const char*> allowedValues) {
    return std::any_of(allowedValues.begin(), allowedValues.end(), [&value](const char* allowedValue) { return value == allowedValue; });
}

#pragma region /* isElectron function */
bool AMaps::isElectron(const std::string& SampleType) { return IsOneOf(SampleType, {"Electron", "electron"}); }
#pragma endregion

// isProton function ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* isProton function */
bool AMaps::isProton(const std::string& SampleType) { return IsOneOf(SampleType, {"Proton", "proton"}); }
#pragma endregion

// isNeutron function ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* isNeutron function */
bool AMaps::isNeutron(const std::string& SampleType) { return IsOneOf(SampleType, {"Neutron", "neutron"}); }
#pragma endregion

// isTL function --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* isTL function */
bool AMaps::isTL(const std::string& SampleType) { return IsOneOf(SampleType, {"Truth", "truth", "TL", "truth level", "truth-level", "Truth-Level"}); }
#pragma endregion

// isReco function --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* isReco function */
bool AMaps::isReco(const std::string& SampleType) { return IsOneOf(SampleType, {"reco", "Reco", "Reconstruction"}); }
#pragma endregion

// hFillMaps function ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* hFillMaps function (G2) */
void AMaps::hFillMaps(const std::string& SampleType, const std::string& particle, double Momentum, double Theta, double Phi, double Weight) {
    bool is_e = isElectron(particle), is_p = isProton(particle), is_n = isNeutron(particle);
    bool is_TL = isTL(SampleType), is_Reco = isReco(SampleType);
    const bool hasElectronSample = bt::FindSubstring(SName, "Uniform_1e_sample_");
    const bool hasProtonSample = bt::FindSubstring(SName, "Uniform_ep_sample_");
    const bool hasNeutronSample = bt::FindSubstring(SName, "Uniform_en_sample_");

    bool TL_e_PrintOut = false, TL_p_PrintOut = false, TL_n_PrintOut = false;
    bool Reco_e_PrintOut = false, Reco_p_PrintOut = false, Reco_n_PrintOut = false;

#pragma region /* Safety checks (AMaps::hFillMaps) */
    if (is_e && is_p && is_n) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle can't be all particles!"); }
    if (!is_e && !is_p && !is_n) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle must be an electron, proton or neutron!"); }
    if (is_e && is_p) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle can't be both electrons and protons!"); }
    if (is_e && is_n) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle can't be both electrons and neutrons!"); }
    if (is_p && is_n) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle can't be both protons and neutrons!"); }

    if (is_TL && is_Reco) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle can't be both TL and Reco!"); }
    if (!is_TL && !is_Reco) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Particle must be either TL or Reco!"); }
#pragma endregion

    auto PrintSliceDebug = [&](bool enabled, const char* limitsName, int sliceIndex, double lower, double upper, const char* particleFlagName, bool particleFlagValue) {
        if (Maps_Mode == "AMaps" && enabled) {
            std::cout << "\n";
            std::cout << limitsName << ".at(" << sliceIndex << ").at(0) = " << lower << "\n";
            std::cout << limitsName << ".at(" << sliceIndex << ").at(1) = " << upper << "\n";
            std::cout << "particle = " << particle << "\n";
            std::cout << particleFlagName << " = " << particleFlagValue << "\n";
            std::cout << "is_TL = " << is_TL << "\n";
            std::cout << "is_Reco = " << is_Reco << "\n";
            std::cout << "Momentum = " << Momentum << "\n";
            std::cout << "Theta = " << Theta << "\n";
            std::cout << "Phi = " << Phi << "\n";
        }
    };

    auto FillFirstMatchingSlice = [&](const std::vector<std::vector<double>>& limits, const char* invalidLimitsErrorMessage, auto&& fillOnMatch, auto&& debugOnMatch) {
        for (int i = 0; i < limits.size(); ++i) {
            const double lower = limits.at(i).at(0);
            const double upper = limits.at(i).at(1);

            if ((Momentum >= lower) && (Momentum < upper)) {
                fillOnMatch(i);
                debugOnMatch(i, lower, upper);

                if (lower > upper) { bt::ExitWithError(__func__, __FILE__, __LINE__, invalidLimitsErrorMessage); }

                break;  // no need to keep the loop going after filling histogram
            }
        }
    };

    if (is_TL) {
        if (is_e && hasElectronSample) {  // electrons are charged -> look for correct momentum slice!
            FillFirstMatchingSlice(
                ElectronMomSliceLimits, "Electron momentum slice limits were set incorrectly!", [&](int i) { truth_theta_e_VS_phi_e_BySlice.at(i).hFill(Phi, Theta, Weight); },
                [&](int i, double lower, double upper) { PrintSliceDebug(TL_e_PrintOut, "ElectronMomSliceLimits", i, lower, upper, "is_e", is_e); });
        } else if (is_p && hasProtonSample) {  // protons are charged -> look for correct momentum slice!
            FillFirstMatchingSlice(
                NucleonMomSliceLimits, "Nucleon momentum slice limits were set incorrectly!", [&](int i) { truth_theta_p_VS_phi_p_BySlice.at(i).hFill(Phi, Theta, Weight); },
                [&](int i, double lower, double upper) { PrintSliceDebug(TL_p_PrintOut, "NucleonMomSliceLimits", i, lower, upper, "is_p", is_p); });
        } else if (is_n && hasNeutronSample) {  // neutrons are neutral -> same fill all slices!
            truth_theta_n_VS_phi_n.hFill(Phi, Theta, Weight);

            for (int Bin = 0; Bin < NucleonMomSliceLimits.size(); Bin++) { truth_theta_n_VS_phi_n_ByConstSlice.at(Bin).hFill(Phi, Theta, Weight); }

            FillFirstMatchingSlice(
                NucleonMomSliceLimits, "Nucleon momentum slice limits were set incorrectly!", [&](int i) { truth_theta_n_VS_phi_n_BySlice.at(i).hFill(Phi, Theta, Weight); },
                [&](int i, double lower, double upper) { PrintSliceDebug(TL_n_PrintOut, "NucleonMomSliceLimits", i, lower, upper, "is_n", is_n); });

            /*
            // truth_theta_n_VS_phi_n.hFill(Phi, Theta, Weight);

            // for (int Bin = 0; Bin < NucleonMomSliceLimits.size(); Bin++) { truth_theta_n_VS_phi_n_BySlice.at(Bin).hFill(Phi, Theta, Weight); }
            */
        }
    } else if (is_Reco) {
        if (is_e && hasElectronSample) {  // electrons are charged -> look for correct momentum slice!
            reco_theta_e_VS_phi_e_BC.hFill(Phi, Theta, Weight);

            FillFirstMatchingSlice(
                ElectronMomSliceLimits, "Electron momentum slice limits were set incorrectly!",
                [&](int i) {
                    reco_theta_e_VS_phi_e_BySlice.at(i).hFill(Phi, Theta, Weight);
                    acceptance_eff_e_BySlice.at(i).hFill(Phi, Theta, Weight);
                    filtered_reco_theta_e_VS_phi_e_BySlice.at(i).hFill(Phi, Theta, Weight);
                },
                [&](int i, double lower, double upper) {
                    // Keep historic debug output as-is ("is_p" in the electron reco block).
                    PrintSliceDebug(Reco_e_PrintOut, "ElectronMomSliceLimits", i, lower, upper, "is_p", is_p);
                });
        } else if (is_p && hasProtonSample) {  // protons are charged -> look for correct momentum slice!
            reco_theta_p_VS_phi_p_BC.hFill(Phi, Theta, Weight);

            FillFirstMatchingSlice(
                NucleonMomSliceLimits, "Nucleon momentum slice limits were set incorrectly!",
                [&](int i) {
                    reco_theta_p_VS_phi_p_BySlice.at(i).hFill(Phi, Theta, Weight);
                    acceptance_eff_p_BySlice.at(i).hFill(Phi, Theta, Weight);
                    filtered_reco_theta_p_VS_phi_p_BySlice.at(i).hFill(Phi, Theta, Weight);
                },
                [&](int i, double lower, double upper) { PrintSliceDebug(Reco_p_PrintOut, "NucleonMomSliceLimits", i, lower, upper, "is_p", is_p); });
        } else if (is_n && hasNeutronSample) {  // neutrons are neutral -> same fill all slices!
            reco_theta_n_VS_phi_n_BC.hFill(Phi, Theta, Weight);

            reco_theta_n_VS_phi_n.hFill(Phi, Theta, Weight);
            acceptance_eff_n.hFill(Phi, Theta, Weight);
            filtered_reco_theta_n_VS_phi_n.hFill(Phi, Theta, Weight);

            for (int Bin = 0; Bin < NucleonMomSliceLimits.size(); Bin++) {
                reco_theta_n_VS_phi_n_ByConstSlice.at(Bin).hFill(Phi, Theta, Weight);
                acceptance_eff_n_ByConstSlice.at(Bin).hFill(Phi, Theta, Weight);
                filtered_reco_theta_n_VS_phi_n_ByConstSlice.at(Bin).hFill(Phi, Theta, Weight);
            }

            FillFirstMatchingSlice(
                NucleonMomSliceLimits, "Nucleon momentum slice limits were set incorrectly!",
                [&](int i) {
                    reco_theta_n_VS_phi_n_BySlice.at(i).hFill(Phi, Theta, Weight);
                    acceptance_eff_n_BySlice.at(i).hFill(Phi, Theta, Weight);
                    filtered_reco_theta_n_VS_phi_n_BySlice.at(i).hFill(Phi, Theta, Weight);
                },
                [&](int i, double lower, double upper) { PrintSliceDebug(Reco_n_PrintOut, "NucleonMomSliceLimits", i, lower, upper, "is_n", is_n); });

            /*
            for (int Bin = 0; Bin < NucleonMomSliceLimits.size(); Bin++) {
                reco_theta_n_VS_phi_n_BySlice.at(Bin).hFill(Phi, Theta, Weight);
                acceptance_eff_n_BySlice.at(Bin).hFill(Phi, Theta, Weight);
                filtered_reco_theta_n_VS_phi_n_BySlice.at(Bin).hFill(Phi, Theta, Weight);
            }
            */
        }
    }
}
#pragma endregion

// CalcAcceptanceEfficiency function --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* CalcAcceptanceEfficiency function (G3a) */
void AMaps::CalcAcceptanceEfficiency() {
    // Calculate electron acceptance efficiency
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {  // Old sample
        std::cout << "\n\nCalculating electron acceptance efficiency...";

        for (int i = 0; i < ElectronMomSliceLimits.size(); i++) {
            acceptance_eff_e_BySlice.at(i).hDivision(truth_theta_e_VS_phi_e_BySlice.at(i).GetHistogram2D());

            if (Maps_Mode == "AMaps") { acceptance_eff_e_BySlice.at(i).ApplyZMaxLim(1.2); }
        }

        std::cout << " done!\n";
    }

    // Calculate proton acceptance efficiency
    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        std::cout << "\n\nCalculating proton acceptance efficiency...";

        for (int i = 0; i < NucleonMomSliceLimits.size(); i++) {
            acceptance_eff_p_BySlice.at(i).hDivision(truth_theta_p_VS_phi_p_BySlice.at(i).GetHistogram2D());

            if (Maps_Mode == "AMaps") { acceptance_eff_p_BySlice.at(i).ApplyZMaxLim(1.2); }
        }

        std::cout << " done!\n";
    }

    // Calculate neutron acceptance efficiency
    if (bt::FindSubstring(SName, "Uniform_en_sample_")) {
        std::cout << "\n\nCalculating neutron acceptance efficiency...";
        acceptance_eff_n.hDivision(truth_theta_n_VS_phi_n.GetHistogram2D());

        // Regular slices
        for (int i = 0; i < NucleonMomSliceLimits.size(); i++) {
            acceptance_eff_n_BySlice.at(i).hDivision(truth_theta_n_VS_phi_n_BySlice.at(i).GetHistogram2D());

            if (Maps_Mode == "AMaps") { acceptance_eff_n_BySlice.at(i).ApplyZMaxLim(1.2); }
        }

        // Constant slices
        for (int i = 0; i < NucleonMomSliceLimits.size(); i++) {
            acceptance_eff_n_ByConstSlice.at(i).hDivision(truth_theta_n_VS_phi_n_ByConstSlice.at(i).GetHistogram2D());

            if (Maps_Mode == "AMaps") { acceptance_eff_n_ByConstSlice.at(i).ApplyZMaxLim(1.2); }
        }

        std::cout << " done!\n";
    }
}
#pragma endregion

// GenerateFilteredRecoMaps function ------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateFilteredRecoMaps function (G3b) */
void AMaps::GenerateFilteredRecoMaps(double cP_minR, double nP_minR) {
    // TODO: generate filtered acceptance efficiency maps instead of filtered theta VS phi plots

    // Generate filtered reco. electron maps
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        for (int bin = 0; bin < ElectronMomSliceLimits.size(); bin++) {
            for (int i = 0; i < (HistElectronSliceNumOfXBins + 1); i++) {
                for (int j = 0; j < (HistElectronSliceNumOfYBins + 1); j++) {
                    if (acceptance_eff_e_BySlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < cP_minR) { filtered_reco_theta_e_VS_phi_e_BySlice.at(bin).hFillByBin(i, j, 0); }
                }
            }
        }
    }

    // Generate filtered reco. proton maps
    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            for (int i = 0; i < (HistNucSliceNumOfXBins + 1); i++) {
                for (int j = 0; j < (HistNucSliceNumOfYBins + 1); j++) {
                    if (acceptance_eff_p_BySlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < cP_minR) { filtered_reco_theta_p_VS_phi_p_BySlice.at(bin).hFillByBin(i, j, 0); }
                }
            }
        }
    }

    // Generate filtered reco. neutron maps
    if (bt::FindSubstring(SName, "Uniform_en_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            for (int i = 0; i < (HistNucSliceNumOfXBins + 1); i++) {
                for (int j = 0; j < (HistNucSliceNumOfYBins + 1); j++) {
                    // Regular slices
                    if (acceptance_eff_n_BySlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < nP_minR) { filtered_reco_theta_n_VS_phi_n_BySlice.at(bin).hFillByBin(i, j, 0); }

                    // Constant slices
                    if (acceptance_eff_n_ByConstSlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < nP_minR) { filtered_reco_theta_n_VS_phi_n_ByConstSlice.at(bin).hFillByBin(i, j, 0); }
                }
            }
        }
    }
}
#pragma endregion

// GenerateMapMatrices function ----------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateMapMatrices function (G3b) */
void AMaps::GenerateMapMatrices(double cP_minR, double nP_minR) {
    // A helper lambda to generate AMap and WMap matrices for a given particle type
    auto GenerateParticleMapMatrices = [&](auto& acceptance_eff_BySlice, int numXBins, int numYBins, double minRatio, std::vector<std::vector<std::vector<int>>>& AMap_Slices,
                                           std::vector<std::vector<std::vector<double>>>& WMap_Slices) {
        for (size_t bin = 0; bin < acceptance_eff_BySlice.size(); bin++) {
            std::vector<std::vector<int>> AMap_Slice;
            std::vector<std::vector<double>> WMap_Slice;

            for (int i = 0; i < numYBins; i++) {
                std::vector<int> AMap_Slice_col;
                std::vector<double> WMap_Slice_col;

                for (int j = 0; j < numXBins; j++) {
                    double binContent = acceptance_eff_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1);
                    if (binContent >= minRatio) {
                        AMap_Slice_col.push_back(1);
                        WMap_Slice_col.push_back(binContent);
                    } else {
                        AMap_Slice_col.push_back(0);
                        WMap_Slice_col.push_back(0.0);
                    }
                }

                AMap_Slice.push_back(AMap_Slice_col);
                WMap_Slice.push_back(WMap_Slice_col);
            }

            AMap_Slices.push_back(AMap_Slice);
            WMap_Slices.push_back(WMap_Slice);
        }
    };

    // Generate electron map matrices
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        GenerateParticleMapMatrices(acceptance_eff_e_BySlice, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins, cP_minR, e_AMap_Slices, e_WMap_Slices);

        e_AMap_Slices_extended = e_AMap_Slices;
    }

    // Generate proton map matrices
    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        GenerateParticleMapMatrices(acceptance_eff_p_BySlice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, cP_minR, p_AMap_Slices, p_WMap_Slices);
    }

    // Generate neutron map matrices
    if (bt::FindSubstring(SName, "Uniform_en_sample_")) {
        GenerateParticleMapMatrices(acceptance_eff_n_BySlice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, nP_minR, n_AMap_Slices, n_WMap_Slices);
        GenerateParticleMapMatrices(acceptance_eff_n_ByConstSlice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, nP_minR, n_AMap_ConstSlices, n_WMap_ConstSlices);
    }
}

// The original implementation for reference:
/*
void AMaps::GenerateMapMatrices(double cP_minR, double nP_minR) {
    // Generate electron map matrices
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        for (int bin = 0; bin < ElectronMomSliceLimits.size(); bin++) {
            // Fill e_AMap_Slices
            std::vector<std::vector<int>> e_AMap_Slice;
            std::vector<std::vector<double>> e_WMap_Slice;

            for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
                std::vector<int> e_AMap_Slice_col;
                std::vector<double> e_WMap_Slice_col;

                for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                    if (acceptance_eff_e_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= cP_minR) {
                        e_AMap_Slice_col.push_back(1);
                        e_WMap_Slice_col.push_back(acceptance_eff_e_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        e_AMap_Slice_col.push_back(0);
                        e_WMap_Slice_col.push_back(0);
                    }
                }

                e_AMap_Slice.push_back(e_AMap_Slice_col);
                e_WMap_Slice.push_back(e_WMap_Slice_col);
            }

            e_AMap_Slices.push_back(e_AMap_Slice);
            e_WMap_Slices.push_back(e_WMap_Slice);
        }

        e_AMap_Slices_extended = e_AMap_Slices;
    }

    // Generate proton map matrices
    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            // Fill p_AMap_Slices
            std::vector<std::vector<int>> p_AMap_Slice;
            std::vector<std::vector<double>> p_WMap_Slice;

            for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                std::vector<int> p_AMap_Slice_col;
                std::vector<double> p_WMap_Slice_col;

                for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                    if (acceptance_eff_p_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= cP_minR) {
                        p_AMap_Slice_col.push_back(1);
                        p_WMap_Slice_col.push_back(acceptance_eff_p_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        p_AMap_Slice_col.push_back(0);
                        p_WMap_Slice_col.push_back(0);
                    }
                }

                p_AMap_Slice.push_back(p_AMap_Slice_col);
                p_WMap_Slice.push_back(p_WMap_Slice_col);
            }

            p_AMap_Slices.push_back(p_AMap_Slice);
            p_WMap_Slices.push_back(p_WMap_Slice);
        }
    }

    // Generate neutron map matrices
    if (bt::FindSubstring(SName, "Uniform_en_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            // Fill n_AMap_Slices (regular Slices)
            std::vector<std::vector<int>> n_AMap_Slice;
            std::vector<std::vector<double>> n_WMap_Slice;

            for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                std::vector<int> n_AMap_Slice_col;
                std::vector<double> n_WMap_Slice_col;

                for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                    if (acceptance_eff_n_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= nP_minR) {
                        n_AMap_Slice_col.push_back(1);
                        n_WMap_Slice_col.push_back(acceptance_eff_n_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        n_AMap_Slice_col.push_back(0);
                        n_WMap_Slice_col.push_back(0);
                    }
                }

                n_AMap_Slice.push_back(n_AMap_Slice_col);
                n_WMap_Slice.push_back(n_WMap_Slice_col);
            }

            n_AMap_Slices.push_back(n_AMap_Slice);
            n_WMap_Slices.push_back(n_WMap_Slice);

            // Fill n_AMap_Slices (constant Slices)
            std::vector<std::vector<int>> n_AMap_ConstSlice;
            std::vector<std::vector<double>> n_WMap_ConstSlice;

            for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                std::vector<int> n_AMap_ConstSlice_col;
                std::vector<double> n_WMap_ConstSlice_col;

                for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                    if (acceptance_eff_n_ByConstSlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= nP_minR) {
                        n_AMap_ConstSlice_col.push_back(1);
                        n_WMap_ConstSlice_col.push_back(acceptance_eff_n_ByConstSlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        n_AMap_ConstSlice_col.push_back(0);
                        n_WMap_ConstSlice_col.push_back(0);
                    }
                }

                n_AMap_ConstSlice.push_back(n_AMap_ConstSlice_col);
                n_WMap_ConstSlice.push_back(n_WMap_ConstSlice_col);
            }

            n_AMap_ConstSlices.push_back(n_AMap_ConstSlice);
            n_WMap_ConstSlices.push_back(n_WMap_ConstSlice);

            // // Generate neutron map matrices (constant Slices)
            // for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            //     n_AMap_ConstSlices.push_back(n_AMap);
            //     n_WMap_ConstSlices.push_back(n_WMap);
            // }
        }
    }
}
*/
#pragma endregion

// GenerateExtendedElectronMapMatrices function ----------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateExtendedElectronMapMatrices function (G3c) */
void AMaps::GenerateExtendedElectronMapMatrices(const std::vector<std::vector<std::vector<int>>>& ref_e_AMap_Slices) {
    // Generate extended electron map matrices
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        std::vector<std::vector<std::vector<int>>> e_AMap_Slices_extended_temp = e_AMap_Slices_extended;

        for (int bin = 0; bin < ElectronMomSliceLimits.size(); bin++) {  // Loop over the slices
            for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
                for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                    // Check if the ref. matrix entry is 1:
                    if (ref_e_AMap_Slices.at(bin).at(i).at(j) == 1) {
                        // Check if the bin is already extended on the sides:
                        if ((i - 1) >= 0) { e_AMap_Slices_extended_temp.at(bin).at(i - 1).at(j) = 1; }

                        if ((i + 1) < HistElectronSliceNumOfYBins) { e_AMap_Slices_extended_temp.at(bin).at(i + 1).at(j) = 1; }

                        if ((j - 1) >= 0) { e_AMap_Slices_extended_temp.at(bin).at(i).at(j - 1) = 1; }

                        if ((j + 1) < HistElectronSliceNumOfXBins) { e_AMap_Slices_extended_temp.at(bin).at(i).at(j + 1) = 1; }

                        // Check if the bin is already extended on the diagonals:
                        if ((i - 1) >= 0 && (j - 1) >= 0) { e_AMap_Slices_extended_temp.at(bin).at(i - 1).at(j - 1) = 1; }

                        if ((i - 1) >= 0 && (j + 1) < HistElectronSliceNumOfXBins) { e_AMap_Slices_extended_temp.at(bin).at(i - 1).at(j + 1) = 1; }

                        if ((i + 1) < HistElectronSliceNumOfYBins && (j - 1) >= 0) { e_AMap_Slices_extended_temp.at(bin).at(i + 1).at(j - 1) = 1; }

                        if ((i + 1) < HistElectronSliceNumOfYBins && (j + 1) < HistElectronSliceNumOfXBins) { e_AMap_Slices_extended_temp.at(bin).at(i + 1).at(j + 1) = 1; }
                    }
                }
            }
        }

        e_AMap_Slices_extended = e_AMap_Slices_extended_temp;
    }
}
#pragma endregion

// GenerateFilteredRecoCPartMaps function ----------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateFilteredRecoCPartMaps function - old (G3ba) */
void AMaps::GenerateFilteredRecoCPartMaps(double cP_minR) {
    // Generate electron map matrices
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        for (int bin = 0; bin < ElectronMomSliceLimits.size(); bin++) {
            for (int i = 0; i < (HistElectronSliceNumOfXBins + 1); i++) {
                for (int j = 0; j < (HistElectronSliceNumOfYBins + 1); j++) {
                    if (acceptance_eff_e_BySlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < cP_minR) { filtered_reco_theta_e_VS_phi_e_BySlice.at(bin).hFillByBin(i, j, 0); }
                }
            }

            // TODO: move from here
            if (Maps_Mode == "AMaps") { acceptance_eff_e_BySlice.at(bin).ApplyZMaxLim(1.2); }

#pragma region /* Fill e_AMap_Slices */
            std::vector<std::vector<int>> e_AMap_slice;
            std::vector<std::vector<double>> e_WMap_slice;

            for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
                std::vector<int> e_AMap_slice_col;
                std::vector<double> e_WMap_slice_col;

                for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                    if (acceptance_eff_e_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= cP_minR) {
                        e_AMap_slice_col.push_back(1);
                        e_WMap_slice_col.push_back(acceptance_eff_e_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        e_AMap_slice_col.push_back(0);
                        e_WMap_slice_col.push_back(0);
                    }
                }

                e_AMap_slice.push_back(e_AMap_slice_col);
                e_WMap_slice.push_back(e_WMap_slice_col);
            }

            e_AMap_Slices.push_back(e_AMap_slice);
            e_WMap_Slices.push_back(e_WMap_slice);
#pragma endregion
        }
    }

    // Generate proton map matrices
    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            for (int i = 0; i < (HistNucSliceNumOfXBins + 1); i++) {
                for (int j = 0; j < (HistNucSliceNumOfYBins + 1); j++) {
                    if (acceptance_eff_p_BySlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < cP_minR) { filtered_reco_theta_p_VS_phi_p_BySlice.at(bin).hFillByBin(i, j, 0); }
                }
            }

            // TODO: move from here
            if (Maps_Mode == "AMaps") { acceptance_eff_p_BySlice.at(bin).ApplyZMaxLim(1.2); }

#pragma region /* Fill p_AMap_Slices */
            std::vector<std::vector<int>> p_AMap_slice;
            std::vector<std::vector<double>> p_WMap_slice;

            for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                std::vector<int> p_AMap_slice_col;
                std::vector<double> p_WMap_slice_col;

                for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                    if (acceptance_eff_p_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= cP_minR) {
                        p_AMap_slice_col.push_back(1);
                        p_WMap_slice_col.push_back(acceptance_eff_p_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        p_AMap_slice_col.push_back(0);
                        p_WMap_slice_col.push_back(0);
                    }
                }

                p_AMap_slice.push_back(p_AMap_slice_col);
                p_WMap_slice.push_back(p_WMap_slice_col);
            }

            p_AMap_Slices.push_back(p_AMap_slice);
            p_WMap_Slices.push_back(p_WMap_slice);
#pragma endregion
        }
    }
}
#pragma endregion

// GenerateCPartAMaps function ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateCPartAMaps function (G3b) */
void AMaps::GenerateCPartAMaps(double cP_minR) {
    GenerateFilteredRecoCPartMaps(cP_minR);

    // Fill electron finalized maps
    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        for (int bin = 0; bin < ElectronMomSliceLimits.size(); bin++) { filtered_reco_theta_e_VS_phi_e.hAdd(filtered_reco_theta_e_VS_phi_e_BySlice.at(bin).GetHistogram2D()); }
    }

    // Fill proton finalized maps
    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) { filtered_reco_theta_p_VS_phi_p.hAdd(filtered_reco_theta_p_VS_phi_p_BySlice.at(bin).GetHistogram2D()); }
    }

    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
            std::vector<int> e_AMap_col;

            for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                if (filtered_reco_theta_e_VS_phi_e.GetHistogram2D()->GetBinContent(j + 1, i + 1) >= cP_minR) {
                    e_AMap_col.push_back(1);
                } else {
                    e_AMap_col.push_back(0);
                }
            }

            e_AMap.push_back(e_AMap_col);
        }
    }

    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
            std::vector<int> p_AMap_col;

            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                if (filtered_reco_theta_p_VS_phi_p.GetHistogram2D()->GetBinContent(j + 1, i + 1) >= cP_minR) {
                    p_AMap_col.push_back(1);
                } else {
                    p_AMap_col.push_back(0);
                }
            }

            p_AMap.push_back(p_AMap_col);
        }
    }
}
#pragma endregion

// GenerateNPartAMaps function ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateNPartAMaps function */
void AMaps::GenerateNPartAMaps(double nP_minR) {
    if (bt::FindSubstring(SName, "Uniform_en_sample_")) {
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            for (int i = 0; i < (HistNucSliceNumOfXBins + 1); i++) {
                for (int j = 0; j < (HistNucSliceNumOfYBins + 1); j++) {
                    if (acceptance_eff_n_BySlice.at(bin).GetHistogram2D()->GetBinContent(i, j) < nP_minR) { filtered_reco_theta_n_VS_phi_n_BySlice.at(bin).hFillByBin(i, j, 0); }
                }
            }

            if (Maps_Mode == "AMaps") {
                // TODO: move from here
                acceptance_eff_n_BySlice.at(bin).ApplyZMaxLim(1.2);
            }

#pragma region /* Fill p_AMap_Slices */
            std::vector<std::vector<int>> n_AMap_slice;
            std::vector<std::vector<double>> n_WMap_slice;

            for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                std::vector<int> n_AMap_slice_col;
                std::vector<double> n_WMap_slice_col;

                for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                    if (acceptance_eff_n_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1) >= nP_minR) {
                        n_AMap_slice_col.push_back(1);
                        n_WMap_slice_col.push_back(acceptance_eff_n_BySlice.at(bin).GetHistogram2D()->GetBinContent(j + 1, i + 1));
                    } else {
                        n_AMap_slice_col.push_back(0);
                        n_WMap_slice_col.push_back(0);
                    }
                }

                n_AMap_slice.push_back(n_AMap_slice_col);
                n_WMap_slice.push_back(n_WMap_slice_col);
            }

            n_AMap_Slices.push_back(n_AMap_slice);
            n_WMap_Slices.push_back(n_WMap_slice);
#pragma endregion
        }

        // TODO: recheck if need n_AMap and n_WMap if we're moving to neutron maps by momentum slices
        for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
            std::vector<int> n_AMap_col;
            std::vector<double> n_WMap_col;

            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                if (acceptance_eff_n.GetHistogram2D()->GetBinContent(j + 1, i + 1) >= nP_minR) {
                    n_AMap_col.push_back(1);
                    n_WMap_col.push_back(acceptance_eff_n.GetHistogram2D()->GetBinContent(j + 1, i + 1));
                } else {
                    n_AMap_col.push_back(0);
                    n_WMap_col.push_back(0);
                }
            }

            n_AMap.push_back(n_AMap_col);
            n_WMap.push_back(n_WMap_col);
        }

        // Fill finalized neutron maps
        // TODO: recheck if filtered_reco_theta_n_VS_phi_n should be here
        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) { filtered_reco_theta_n_VS_phi_n_BySlice.push_back(filtered_reco_theta_n_VS_phi_n); }

        /*
        for (int i = 0; i < (HistNucSliceNumOfXBins + 1); i++) {
            for (int j = 0; j < (HistNucSliceNumOfYBins + 1); j++) {
                if (acceptance_eff_n.GetHistogram2D()->GetBinContent(i, j) < nP_minR) {
                    filtered_reco_theta_n_VS_phi_n.hFillByBin(i, j, 0);

                    for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) { filtered_reco_theta_n_VS_phi_n_BySlice.at(bin).hFillByBin(i, j, 0); }
                }
            }
        }

        if (Maps_Mode == "AMaps") {
            for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) { acceptance_eff_n_BySlice.at(bin).ApplyZMaxLim(1.2); }

            acceptance_eff_n.ApplyZMaxLim(1.2);
        }

        for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
            std::vector<int> n_AMap_col;
            std::vector<double> n_WMap_col;

            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                if (acceptance_eff_n.GetHistogram2D()->GetBinContent(j + 1, i + 1) >= nP_minR) {
                    n_AMap_col.push_back(1);
                    n_WMap_col.push_back(acceptance_eff_n.GetHistogram2D()->GetBinContent(j + 1, i + 1));
                } else {
                    n_AMap_col.push_back(0);
                    n_WMap_col.push_back(0);
                }
            }

            n_AMap.push_back(n_AMap_col);
            n_WMap.push_back(n_WMap_col);
        }

        for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
            filtered_reco_theta_n_VS_phi_n_BySlice.push_back(filtered_reco_theta_n_VS_phi_n);
            n_AMap_Slices.push_back(n_AMap);
            n_WMap_Slices.push_back(n_WMap);
        }
        */
    }
}
#pragma endregion

// GenerateNucleonAMap function -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GenerateNucleonAMap function - old (G3d) */
void AMaps::GenerateNucleonAMap() {
    for (int i = 0; i < (HistNucSliceNumOfXBins + 1); i++) {
        for (int j = 0; j < (HistNucSliceNumOfYBins + 1); j++) {
            if ((reco_theta_n_VS_phi_n_BC.GetHistogram2D()->GetBinContent(i, j) != 0) && (reco_theta_p_VS_phi_p_BC.GetHistogram2D()->GetBinContent(i, j) != 0)) {
                reco_theta_nuc_VS_phi_nuc_BC.hFillByBin(i, j, reco_theta_n_VS_phi_n.GetHistogram2D()->GetBinContent(i, j));
            }

            if ((filtered_reco_theta_n_VS_phi_n.GetHistogram2D()->GetBinContent(i, j) != 0) && (filtered_reco_theta_p_VS_phi_p.GetHistogram2D()->GetBinContent(i, j) != 0)) {
                filtered_reco_theta_nuc_VS_phi_nuc.hFillByBin(i, j, reco_theta_n_VS_phi_n.GetHistogram2D()->GetBinContent(i, j));
            }
        }
    }

    for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
        std::vector<int> nuc_col;

        for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
            if (filtered_reco_theta_nuc_VS_phi_nuc.GetHistogram2D()->GetBinContent(j + 1, i + 1) != 0) {
                nuc_col.push_back(1);
            } else {
                nuc_col.push_back(0);
            }
        }

        nuc_AMap.push_back(nuc_col);
    }

    for (int bin = 0; bin < NucleonMomSliceLimits.size(); bin++) {
        std::vector<std::vector<int>> nuc_slice;

        for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
            std::vector<int> nuc_col;

            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                if ((p_AMap_Slices.at(bin).at(i).at(j) == 1) && (n_AMap_Slices.at(bin).at(i).at(j) == 1)) {
                    nuc_col.push_back(1);
                } else {
                    nuc_col.push_back(0);
                }
            }

            nuc_slice.push_back(nuc_col);
        }

        nuc_AMap_Slices.push_back(nuc_slice);
        nuc_WMap_Slices.push_back(nuc_slice);  // TODO: figure out if really need these!
    }
}
#pragma endregion

// SaveMaps function -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* SaveMaps function (G3e) */

void AMaps::SaveMaps(const std::string& SampleName, const std::string& AcceptanceMapsDirectory) {
    bool PrintOut = false;

    int testNumber = 0;
    const bool saveAMaps = (Maps_Mode == "AMaps");
    const bool saveWMaps = (Maps_Mode == "WMaps");
    const std::string modeLabel = Maps_Mode;

    if (!saveAMaps && !saveWMaps) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Unsupported Maps_Mode: " + Maps_Mode); }

    auto ensureDir = [](const std::string& path) { system(("mkdir -p " + path).c_str()); };
    auto copyToDir = [](const std::string& sourceFile, const std::string& targetDir) { system(("cp " + sourceFile + " " + targetDir).c_str()); };

    auto sliceFileName = [&](const std::string& particleTag, const std::vector<std::vector<double>>& limits, int slice) {
        return particleTag + "_" + modeLabel + "_file_from_" + bt::ToStringWithPrecision(limits.at(slice).at(0), 2) + "_to_" + bt::ToStringWithPrecision(limits.at(slice).at(1), 2) + ".par";
    };

    auto writeSliceMatrixFile = [&](const std::string& filePath, const std::vector<std::vector<double>>& limits, int slice, int xBins, int yBins, const char* xBinsLabel,
                                    const char* yBinsLabel, const char* rowPrefix, const auto& slicesContainer) {
        std::ofstream tempFile(filePath);
        tempFile << "Lower_P_lim:\t" << limits.at(slice).at(0) << "\n";
        tempFile << "Upper_P_lim:\t" << limits.at(slice).at(1) << "\n";
        tempFile << "\n";
        tempFile << xBinsLabel << ":\t" << xBins << "\n";
        tempFile << yBinsLabel << ":\t" << yBins << "\n";
        tempFile << "\n";

        for (int i = 0; i < yBins; i++) {
            tempFile << rowPrefix << "\t";
            for (int j = 0; j < xBins; j++) {
                tempFile << slicesContainer.at(slice).at(i).at(j);
                if (j != xBins - 1) { tempFile << ":"; }
            }
            tempFile << "\n";
        }
    };

    auto writeElectronExtendedSlice = [&](const std::string& filePath, const std::vector<std::vector<double>>& limits, int slice) {
        std::ofstream tempFile(filePath);
        tempFile << "Lower_P_lim:\t" << limits.at(slice).at(0) << "\n";
        tempFile << "Upper_P_lim:\t" << limits.at(slice).at(1) << "\n";
        tempFile << "\n";

        for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
            tempFile << "Line\t";
            for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                tempFile << e_AMap_Slices_extended.at(slice).at(i).at(j);
                if (j != HistElectronSliceNumOfXBins - 1) { tempFile << ":"; }
            }
            tempFile << "\n";
        }
    };

    auto writeSliceLimits = [&](const std::string& particleTag, const std::vector<std::vector<double>>& limits, const std::string& savePath, const std::string& savePathCopy) {
        const std::string limitsFileName = particleTag + "_slice_limits.par";
        const std::string fullPath = savePath + limitsFileName;
        std::ofstream limitsFile(fullPath);

        for (int slice = 0; slice < limits.size(); slice++) {
            limitsFile << particleTag << "_slice_" << (slice + 1) << "\t" << limits.at(slice).at(0) << ":" << limits.at(slice).at(1) << "\n";
        }
        limitsFile << "\n";
        limitsFile.close();
        copyToDir(fullPath, savePathCopy);
    };

    auto makeModePaths = [&](const std::string& particleTag, const std::string& suffix) {
        const std::string savePath = AcceptanceMapsDirectory + SampleName + "/" + particleTag + "_" + modeLabel + suffix;
        const std::string copyPath = AMapCopySavePath + "/" + particleTag + "_" + modeLabel + suffix;
        ensureDir(savePath);
        ensureDir(copyPath);
        return std::pair<std::string, std::string>{savePath, copyPath};
    };

    if (bt::FindSubstring(SName, "Uniform_1e_sample_")) {
        auto [electronSavePath, electronCopyPath] = makeModePaths("e", "_by_slice/");

        for (int slice = 0; slice < ElectronMomSliceLimits.size(); slice++) {
            const std::string fileName = sliceFileName("e", ElectronMomSliceLimits, slice);
            const std::string fullPath = electronSavePath + fileName;

            if (saveAMaps) {
                writeSliceMatrixFile(fullPath, ElectronMomSliceLimits, slice, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins, "HistElectronSliceNumOfXBins",
                                     "HistElectronSliceNumOfYBins", "Line", e_AMap_Slices);
            } else {
                writeSliceMatrixFile(fullPath, ElectronMomSliceLimits, slice, HistElectronSliceNumOfXBins, HistElectronSliceNumOfYBins, "HistElectronSliceNumOfXBins",
                                     "HistElectronSliceNumOfYBins", "Weight", e_WMap_Slices);
            }

            copyToDir(fullPath, electronCopyPath);
        }

        writeSliceLimits("e", ElectronMomSliceLimits, electronSavePath, electronCopyPath);

        if (saveAMaps) {
            const std::string extendedSavePath = AcceptanceMapsDirectory + SampleName + "/e_extended_" + Maps_Mode + "_by_slice/";
            const std::string extendedCopyPath = AMapCopySavePath + "/e_extended_" + Maps_Mode + "_by_slice/";
            ensureDir(extendedSavePath);
            ensureDir(extendedCopyPath);

            for (int slice = 0; slice < ElectronMomSliceLimits.size(); slice++) {
                const std::string fileName = "e_" + Maps_Mode + "_file_from_" + bt::ToStringWithPrecision(ElectronMomSliceLimits.at(slice).at(0), 2) + "_to_" +
                                             bt::ToStringWithPrecision(ElectronMomSliceLimits.at(slice).at(1), 2) + ".par";
                const std::string fullPath = extendedSavePath + fileName;
                writeElectronExtendedSlice(fullPath, ElectronMomSliceLimits, slice);
                copyToDir(fullPath, extendedCopyPath);
            }
        }
    }

    if (bt::FindSubstring(SName, "Uniform_ep_sample_")) {
        auto [protonSavePath, protonCopyPath] = makeModePaths("p", "_by_slice/");

        for (int slice = 0; slice < NucleonMomSliceLimits.size(); slice++) {
            const std::string fileName = sliceFileName("p", NucleonMomSliceLimits, slice);
            const std::string fullPath = protonSavePath + fileName;

            if (saveAMaps) {
                writeSliceMatrixFile(fullPath, NucleonMomSliceLimits, slice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, "HistNucSliceNumOfXBins", "HistNucSliceNumOfYBins", "Line",
                                     p_AMap_Slices);
            } else {
                writeSliceMatrixFile(fullPath, NucleonMomSliceLimits, slice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, "HistNucSliceNumOfXBins", "HistNucSliceNumOfYBins", "Weight",
                                     p_WMap_Slices);
            }

            copyToDir(fullPath, protonCopyPath);
        }

        writeSliceLimits("p", NucleonMomSliceLimits, protonSavePath, protonCopyPath);
    }

    if (bt::FindSubstring(SName, "Uniform_en_sample_")) {
        auto [neutronSavePath, neutronCopyPath] = makeModePaths("n", "_by_slice/");
        auto [neutronConstSavePath, neutronConstCopyPath] = makeModePaths("n", "_by_ConstSlice/");

        for (int slice = 0; slice < NucleonMomSliceLimits.size(); slice++) {
            if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

            const std::string fileName = sliceFileName("n", NucleonMomSliceLimits, slice);
            const std::string fullPath = neutronSavePath + fileName;
            const std::string fullPathConst = neutronConstSavePath + fileName;

            if (saveAMaps) {
                writeSliceMatrixFile(fullPath, NucleonMomSliceLimits, slice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, "HistNucSliceNumOfXBins", "HistNucSliceNumOfYBins", "Line",
                                     n_AMap_Slices);
                writeSliceMatrixFile(fullPathConst, NucleonMomSliceLimits, slice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, "HistNucSliceNumOfXBins", "HistNucSliceNumOfYBins", "Line",
                                     n_AMap_ConstSlices);
            } else {
                writeSliceMatrixFile(fullPath, NucleonMomSliceLimits, slice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, "HistNucSliceNumOfXBins", "HistNucSliceNumOfYBins", "Weight",
                                     n_WMap_Slices);
                writeSliceMatrixFile(fullPathConst, NucleonMomSliceLimits, slice, HistNucSliceNumOfXBins, HistNucSliceNumOfYBins, "HistNucSliceNumOfXBins", "HistNucSliceNumOfYBins",
                                     "Weight", n_WMap_ConstSlices);
            }

            copyToDir(fullPath, neutronCopyPath);
            copyToDir(fullPathConst, neutronConstCopyPath);
        }

        writeSliceLimits("n", NucleonMomSliceLimits, neutronSavePath, neutronCopyPath);
        writeSliceLimits("n", NucleonMomSliceLimits, neutronConstSavePath, neutronConstCopyPath);
    }
}
#pragma endregion

// SaveHitMaps function -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* SaveHitMaps function - old (G3d) */

// TODO: separate into AMaps and WMaps

void AMaps::SaveHitMaps(const std::string& SampleName, const std::string& AcceptanceMapsDirectory) {
    bool PrintOut = false;

    int testNumber = 0;

    // A helper lambda function that returns the 4 paths and creates the directories
    auto makeSliceDirs = [&](const std::string& particle) -> std::tuple<std::string, std::string, std::string, std::string> {
        const std::string aMapPath = AcceptanceMapsDirectory + SampleName + "/" + particle + "_" + Maps_Mode + "_by_slice/";
        system(("mkdir -p " + aMapPath).c_str());

        const std::string wMapPath = AcceptanceMapsDirectory + SampleName + "/" + particle + "_WMap_by_slice/";
        system(("mkdir -p " + wMapPath).c_str());

        const std::string aMapCopyPath = AMapCopySavePath + "/" + particle + "_" + Maps_Mode + "_by_slice/";
        system(("mkdir -p " + aMapCopyPath).c_str());

        const std::string wMapCopyPath = AMapCopySavePath + "/" + particle + "_WMap_by_slice/";
        system(("mkdir -p " + wMapCopyPath).c_str());

        return {aMapPath, wMapPath, aMapCopyPath, wMapCopyPath};
    };

    // Bind the 4-tuples into the exact variable names your code already uses:
    auto [AMapSliceElectronSavePath, WMapSliceElectronSavePath, AMapSliceElectronSavePathCopy, WMapSliceElectronSavePathCopy] = makeSliceDirs("e");
    auto [AMapSliceProtonSavePath, WMapSliceProtonSavePath, AMapSliceProtonSavePathCopy, WMapSliceProtonSavePathCopy] = makeSliceDirs("p");
    auto [AMapSliceNeutronSavePath, WMapSliceNeutronSavePath, AMapSliceNeutronSavePathCopy, WMapSliceNeutronSavePathCopy] = makeSliceDirs("n");
    auto [AMapSliceNucleonSavePath, WMapSliceNucleonSavePath, AMapSliceNucleonSavePathCopy, WMapSliceNucleonSavePathCopy] = makeSliceDirs("nuc");

    // std::string AMapSliceElectronSavePath = AcceptanceMapsDirectory + SampleName + "/e_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceElectronSavePath).c_str());
    // std::string WMapSliceElectronSavePath = AcceptanceMapsDirectory + SampleName + "/e_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceElectronSavePath).c_str());
    // std::string AMapSliceElectronSavePathCopy = AMapCopySavePath + "/e_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceElectronSavePathCopy).c_str());
    // std::string WMapSliceElectronSavePathCopy = AMapCopySavePath + "/e_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceElectronSavePathCopy).c_str());

    // std::string AMapSliceProtonSavePath = AcceptanceMapsDirectory + SampleName + "/p_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceProtonSavePath).c_str());
    // std::string WMapSliceProtonSavePath = AcceptanceMapsDirectory + SampleName + "/p_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceProtonSavePath).c_str());
    // std::string AMapSliceProtonSavePathCopy = AMapCopySavePath + "/p_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceProtonSavePathCopy).c_str());
    // std::string WMapSliceProtonSavePathCopy = AMapCopySavePath + "/p_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceProtonSavePathCopy).c_str());

    // std::string AMapSliceNeutronSavePath = AcceptanceMapsDirectory + SampleName + "/n_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceNeutronSavePath).c_str());
    // std::string WMapSliceNeutronSavePath = AcceptanceMapsDirectory + SampleName + "/n_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceNeutronSavePath).c_str());
    // std::string AMapSliceNeutronSavePathCopy = AMapCopySavePath + "/n_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceNeutronSavePathCopy).c_str());
    // std::string WMapSliceNeutronSavePathCopy = AMapCopySavePath + "/n_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceNeutronSavePathCopy).c_str());

    // std::string AMapSliceNucleonSavePath = AcceptanceMapsDirectory + SampleName + "/nuc_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceNucleonSavePath).c_str());
    // std::string WMapSliceNucleonSavePath = AcceptanceMapsDirectory + SampleName + "/nuc_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceNucleonSavePath).c_str());
    // std::string AMapSliceNucleonSavePathCopy = AMapCopySavePath + "/nuc_" + Maps_Mode + "_by_slice/";
    // system(("mkdir -p " + AMapSliceNucleonSavePathCopy).c_str());
    // std::string WMapSliceNucleonSavePathCopy = AMapCopySavePath + "/nuc_WMap_by_slice/";
    // system(("mkdir -p " + WMapSliceNucleonSavePathCopy).c_str());

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

#pragma region /* Save electron slices */
    for (int Slice = 0; Slice < ElectronMomSliceLimits.size(); Slice++) {
        std::ofstream e_AMap_TempFile, e_WMap_TempFile;

        std::string AMapTempFileName = "e_" + Maps_Mode + "_file_from_" + bt::ToStringWithPrecision(ElectronMomSliceLimits.at(Slice).at(0), 2) + "_to_" +
                                       bt::ToStringWithPrecision(ElectronMomSliceLimits.at(Slice).at(1), 2) + ".par";
        std::string WMapTempFileName = "e_WMap_file_from_" + bt::ToStringWithPrecision(ElectronMomSliceLimits.at(Slice).at(0), 2) + "_to_" +
                                       bt::ToStringWithPrecision(ElectronMomSliceLimits.at(Slice).at(1), 2) + ".par";

        e_AMap_TempFile = std::ofstream(AMapSliceElectronSavePath + AMapTempFileName);  // Opens in overwrite mode
        e_WMap_TempFile = std::ofstream(WMapSliceElectronSavePath + WMapTempFileName);

        e_AMap_TempFile << "Lower_P_lim:\t" << ElectronMomSliceLimits.at(Slice).at(0) << "\n";
        e_AMap_TempFile << "Upper_P_lim:\t" << ElectronMomSliceLimits.at(Slice).at(1) << "\n";
        e_AMap_TempFile << "\n";

        e_WMap_TempFile << "Lower_P_lim:\t" << ElectronMomSliceLimits.at(Slice).at(0) << "\n";
        e_WMap_TempFile << "Upper_P_lim:\t" << ElectronMomSliceLimits.at(Slice).at(1) << "\n";
        e_WMap_TempFile << "\n";

        for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
            e_AMap_TempFile << "Line\t";
            e_WMap_TempFile << "Weight\t";

            for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                if (j != HistElectronSliceNumOfXBins - 1) {
                    e_AMap_TempFile << e_AMap_Slices.at(Slice).at(i).at(j) << ":";
                    e_WMap_TempFile << e_WMap_Slices.at(Slice).at(i).at(j) << ":";
                } else {
                    e_AMap_TempFile << e_AMap_Slices.at(Slice).at(i).at(j);
                    e_WMap_TempFile << e_WMap_Slices.at(Slice).at(i).at(j);
                }
            }

            e_AMap_TempFile << "\n";
            e_WMap_TempFile << "\n";
        }

        e_AMap_TempFile.close();
        e_WMap_TempFile.close();

        system(("cp " + AMapSliceElectronSavePath + AMapTempFileName + " " + AMapSliceElectronSavePathCopy + AMapTempFileName).c_str());
        system(("cp " + WMapSliceElectronSavePath + WMapTempFileName + " " + WMapSliceElectronSavePathCopy + WMapTempFileName).c_str());
    }
#pragma endregion

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

#pragma region /* Save proton, neutron & nucleon slices */
    for (int Slice = 0; Slice < NucleonMomSliceLimits.size(); Slice++) {
        std::ofstream p_AMap_TempFile, p_WMap_TempFile, n_AMap_TempFile, n_WMap_TempFile, nuc_AMap_TempFile, nuc_WMap_TempFile;

        std::string ProtonAMapTempFileName = "p_" + Maps_Mode + "_file_from_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(0), 2) + "_to_" +
                                             bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(1), 2) + ".par";
        std::string ProtonWMapTempFileName =
            "p_WMap_file_from_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(0), 2) + "_to_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(1), 2) + ".par";
        std::string NeutronAMapTempFileName = "n_" + Maps_Mode + "_file_from_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(0), 2) + "_to_" +
                                              bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(1), 2) + ".par";
        std::string NeutronWMapTempFileName =
            "n_WMap_file_from_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(0), 2) + "_to_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(1), 2) + ".par";
        std::string NucleonAMapTempFileName = "nuc_" + Maps_Mode + "_file_from_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(0), 2) + "_to_" +
                                              bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(1), 2) + ".par";
        std::string NucleonWMapTempFileName = "nuc_WMap_file_from_" + bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(0), 2) + "_to_" +
                                              bt::ToStringWithPrecision(NucleonMomSliceLimits.at(Slice).at(1), 2) + ".par";

        p_AMap_TempFile = std::ofstream(AMapSliceProtonSavePath + ProtonAMapTempFileName);  // Opens in overwrite mode
        p_WMap_TempFile = std::ofstream(WMapSliceProtonSavePath + ProtonWMapTempFileName);
        n_AMap_TempFile = std::ofstream(AMapSliceNeutronSavePath + NeutronAMapTempFileName);
        n_WMap_TempFile = std::ofstream(WMapSliceNeutronSavePath + NeutronWMapTempFileName);
        nuc_AMap_TempFile = std::ofstream(AMapSliceNucleonSavePath + NucleonAMapTempFileName);
        nuc_WMap_TempFile = std::ofstream(WMapSliceNucleonSavePath + NucleonWMapTempFileName);

        p_AMap_TempFile << "Lower_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(0) << "\n";
        p_AMap_TempFile << "Upper_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        p_AMap_TempFile << "\n";
        n_AMap_TempFile << "Lower_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(0) << "\n";
        n_AMap_TempFile << "Upper_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        n_AMap_TempFile << "\n";
        nuc_AMap_TempFile << "Lower_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(0) << "\n";
        nuc_AMap_TempFile << "Upper_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        nuc_AMap_TempFile << "\n";

        p_WMap_TempFile << "Lower_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(0) << "\n";
        p_WMap_TempFile << "Upper_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        p_WMap_TempFile << "\n";
        n_WMap_TempFile << "Lower_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(0) << "\n";
        n_WMap_TempFile << "Upper_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        n_WMap_TempFile << "\n";
        nuc_WMap_TempFile << "Lower_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(0) << "\n";
        nuc_WMap_TempFile << "Upper_P_lim:\t" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        nuc_WMap_TempFile << "\n";

        for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
            p_AMap_TempFile << "Line\t";
            p_WMap_TempFile << "Weight\t";
            n_AMap_TempFile << "Line\t";
            n_WMap_TempFile << "Weight\t";
            nuc_AMap_TempFile << "Line\t";
            nuc_WMap_TempFile << "Weight\t";

            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                if (j != HistNucSliceNumOfXBins - 1) {
                    p_AMap_TempFile << p_AMap_Slices.at(Slice).at(i).at(j) << ":";
                    p_WMap_TempFile << p_WMap_Slices.at(Slice).at(i).at(j) << ":";
                    n_AMap_TempFile << n_AMap_Slices.at(Slice).at(i).at(j) << ":";
                    n_WMap_TempFile << n_WMap_Slices.at(Slice).at(i).at(j) << ":";
                    nuc_AMap_TempFile << nuc_AMap_Slices.at(Slice).at(i).at(j) << ":";
                    nuc_WMap_TempFile << nuc_WMap_Slices.at(Slice).at(i).at(j) << ":";
                } else {
                    p_AMap_TempFile << p_AMap_Slices.at(Slice).at(i).at(j);
                    p_WMap_TempFile << p_WMap_Slices.at(Slice).at(i).at(j);
                    n_AMap_TempFile << n_AMap_Slices.at(Slice).at(i).at(j);
                    n_WMap_TempFile << n_WMap_Slices.at(Slice).at(i).at(j);
                    nuc_AMap_TempFile << nuc_AMap_Slices.at(Slice).at(i).at(j);
                    nuc_WMap_TempFile << nuc_WMap_Slices.at(Slice).at(i).at(j);
                }
            }

            p_AMap_TempFile << "\n";
            p_WMap_TempFile << "\n";
            n_AMap_TempFile << "\n";
            n_WMap_TempFile << "\n";
            nuc_AMap_TempFile << "\n";
            nuc_WMap_TempFile << "\n";
        }

        p_AMap_TempFile.close();
        p_WMap_TempFile.close();
        n_AMap_TempFile.close();
        n_WMap_TempFile.close();
        nuc_AMap_TempFile.close();
        nuc_WMap_TempFile.close();

        system(("cp " + AMapSliceProtonSavePath + ProtonAMapTempFileName + " " + AMapSliceProtonSavePathCopy + ProtonAMapTempFileName).c_str());
        system(("cp " + WMapSliceProtonSavePath + ProtonWMapTempFileName + " " + WMapSliceProtonSavePathCopy + ProtonWMapTempFileName).c_str());
        system(("cp " + AMapSliceNeutronSavePath + NeutronAMapTempFileName + " " + AMapSliceNeutronSavePathCopy + NeutronAMapTempFileName).c_str());
        system(("cp " + WMapSliceNeutronSavePath + NeutronWMapTempFileName + " " + WMapSliceNeutronSavePathCopy + NeutronWMapTempFileName).c_str());
        system(("cp " + AMapSliceNucleonSavePath + NucleonAMapTempFileName + " " + AMapSliceNucleonSavePathCopy + NucleonAMapTempFileName).c_str());
        system(("cp " + WMapSliceNucleonSavePath + NucleonWMapTempFileName + " " + WMapSliceNucleonSavePathCopy + NucleonWMapTempFileName).c_str());
    }
#pragma endregion

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

#pragma region /* Save combined maps */
    // TODO: figure out rather or not to keep these combind maps!
    std::ofstream e_AMap_file, p_AMap_file, n_AMap_file, nuc_AMap_file;

    e_AMap_file = std::ofstream(AcceptanceMapsDirectory + SampleName + "/e_" + Maps_Mode + "_file.par");  // Opens in overwrite mode
    p_AMap_file = std::ofstream(AcceptanceMapsDirectory + SampleName + "/p_" + Maps_Mode + "_file.par");
    n_AMap_file = std::ofstream(AcceptanceMapsDirectory + SampleName + "/n_" + Maps_Mode + "_file.par");
    nuc_AMap_file = std::ofstream(AcceptanceMapsDirectory + SampleName + "/nuc_" + Maps_Mode + "_file.par");

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    for (int Slice = 0; Slice < ElectronMomSliceLimits.size(); Slice++) {
        e_AMap_file << "e_slice_" << (Slice + 1) << "\t" << ElectronMomSliceLimits.at(Slice).at(0) << ":" << ElectronMomSliceLimits.at(Slice).at(1) << "\n";
    }

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    for (int Slice = 0; Slice < NucleonMomSliceLimits.size(); Slice++) {
        p_AMap_file << "p_slice_" << (Slice + 1) << "\t" << NucleonMomSliceLimits.at(Slice).at(0) << ":" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        n_AMap_file << "n_slice_" << (Slice + 1) << "\t" << NucleonMomSliceLimits.at(Slice).at(0) << ":" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        nuc_AMap_file << "nuc_slice_" << (Slice + 1) << "\t" << NucleonMomSliceLimits.at(Slice).at(0) << ":" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
    }

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
        e_AMap_file << "Line\t";

        for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
            if (j != HistElectronSliceNumOfXBins - 1) {
                e_AMap_file << e_AMap.at(i).at(j) << ":";
            } else {
                e_AMap_file << e_AMap.at(i).at(j);
            }
        }

        e_AMap_file << "\n";
    }

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
        p_AMap_file << "Line\t";
        n_AMap_file << "Line\t";
        nuc_AMap_file << "Line\t";

        for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
            if (j != HistNucSliceNumOfXBins - 1) {
                if (PrintOut) { std::cout << "\n\np_" + Maps_Mode + "_file << p_AMap.at(" << i << ").at(" << j << ") <<\n"; }
                p_AMap_file << p_AMap.at(i).at(j) << ":";
                if (PrintOut) { std::cout << "\n\nn_" + Maps_Mode + "_file << n_AMap.at(" << i << ").at(" << j << ") <<\n"; }
                n_AMap_file << n_AMap.at(i).at(j) << ":";
                if (PrintOut) { std::cout << "\n\nnuc_" + Maps_Mode + "_file << nuc_AMap.at(" << i << ").at(" << j << ") <<\n"; }
                nuc_AMap_file << nuc_AMap.at(i).at(j) << ":";
            } else {
                if (PrintOut) { std::cout << "\n\np_" + Maps_Mode + "_file << p_AMap.at(" << i << ").at(" << j << ") <<\n"; }
                p_AMap_file << p_AMap.at(i).at(j);
                if (PrintOut) { std::cout << "\n\nn_" + Maps_Mode + "_file << n_AMap.at(" << i << ").at(" << j << ") <<\n"; }
                n_AMap_file << n_AMap.at(i).at(j);
                if (PrintOut) { std::cout << "\n\nnuc_" + Maps_Mode + "_file << nuc_AMap.at(" << i << ").at(" << j << ") <<\n"; }
                nuc_AMap_file << nuc_AMap.at(i).at(j);
            }
        }

        p_AMap_file << "\n";
        n_AMap_file << "\n";
        nuc_AMap_file << "\n";
    }

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    e_AMap_file.close();
    p_AMap_file.close();
    n_AMap_file.close();
    nuc_AMap_file.close();

    system(("cp " + AcceptanceMapsDirectory + SampleName + "/e_" + Maps_Mode + "_file.par " + AMapCopySavePath).c_str());
    system(("cp " + AcceptanceMapsDirectory + SampleName + "/p_" + Maps_Mode + "_file.par " + AMapCopySavePath).c_str());
    system(("cp " + AcceptanceMapsDirectory + SampleName + "/n_" + Maps_Mode + "_file.par " + AMapCopySavePath).c_str());
    system(("cp " + AcceptanceMapsDirectory + SampleName + "/nuc_" + Maps_Mode + "_file.par " + AMapCopySavePath).c_str());
#pragma endregion

#pragma region /* Slice limits */
    std::ofstream e_slice_limits, p_slice_limits, n_slice_limits, nuc_slice_limits;

    // e_slice_limits.open(AMapSliceElectronSavePath + "e_slice_limits.par");
    // p_slice_limits.open(AMapSliceProtonSavePath + "p_slice_limits.par");
    // n_slice_limits.open(AMapSliceNeutronSavePath + "n_slice_limits.par");
    // nuc_slice_limits.open(AMapSliceNucleonSavePath + "nuc_slice_limits.par");
    e_slice_limits = std::ofstream(AMapSliceElectronSavePath + "e_slice_limits.par");  // Opens in overwrite mode
    p_slice_limits = std::ofstream(AMapSliceProtonSavePath + "p_slice_limits.par");
    n_slice_limits = std::ofstream(AMapSliceNeutronSavePath + "n_slice_limits.par");
    nuc_slice_limits = std::ofstream(AMapSliceNucleonSavePath + "nuc_slice_limits.par");

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    for (int Slice = 0; Slice < ElectronMomSliceLimits.size(); Slice++) {
        e_slice_limits << "e_slice_" << (Slice + 1) << "\t" << ElectronMomSliceLimits.at(Slice).at(0) << ":" << ElectronMomSliceLimits.at(Slice).at(1) << "\n";
    }

    if (PrintOut) { ++testNumber, std::cout << "\n\n\nTEST " << testNumber << "\n"; }

    for (int Slice = 0; Slice < NucleonMomSliceLimits.size(); Slice++) {
        p_slice_limits << "p_slice_" << (Slice + 1) << "\t" << NucleonMomSliceLimits.at(Slice).at(0) << ":" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        n_slice_limits << "n_slice_" << (Slice + 1) << "\t" << NucleonMomSliceLimits.at(Slice).at(0) << ":" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
        nuc_slice_limits << "nuc_slice_" << (Slice + 1) << "\t" << NucleonMomSliceLimits.at(Slice).at(0) << ":" << NucleonMomSliceLimits.at(Slice).at(1) << "\n";
    }

    e_slice_limits << "\n";
    p_slice_limits << "\n";
    n_slice_limits << "\n";
    nuc_slice_limits << "\n";

    e_slice_limits.close();
    p_slice_limits.close();
    n_slice_limits.close();
    nuc_slice_limits.close();

    system(("cp " + AMapSliceElectronSavePath + "e_slice_limits.par " + AMapSliceElectronSavePathCopy).c_str());
    system(("cp " + AMapSliceProtonSavePath + "p_slice_limits.par " + AMapSliceProtonSavePathCopy).c_str());
    system(("cp " + AMapSliceNeutronSavePath + "n_slice_limits.par " + AMapSliceNeutronSavePathCopy).c_str());
    system(("cp " + AMapSliceNucleonSavePath + "nuc_slice_limits.par " + AMapSliceNucleonSavePathCopy).c_str());

    system(("cp " + WMapSliceElectronSavePath + "e_slice_limits.par " + WMapSliceElectronSavePathCopy).c_str());
    system(("cp " + WMapSliceProtonSavePath + "p_slice_limits.par " + WMapSliceProtonSavePathCopy).c_str());
    system(("cp " + WMapSliceNeutronSavePath + "n_slice_limits.par " + WMapSliceNeutronSavePathCopy).c_str());
    system(("cp " + WMapSliceNucleonSavePath + "nuc_slice_limits.par " + WMapSliceNucleonSavePathCopy).c_str());
#pragma endregion
}
#pragma endregion

// DrawAndSaveMapsBySliceToPDFs function ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* DrawAndSaveMapsBySliceToPDFs function */
void AMaps::DrawAndSaveMapsBySliceToPDFs(std::vector<TObject*> HistoList, const std::string& PDFfileName) {
    TCanvas* myText_temp = new TCanvas("myText_temp", "myText_temp", pixelx, pixely);
    TCanvas* myCanvas_temp = new TCanvas("myCanvas_temp", "myCanvas_temp", pixelx, pixely);

    gStyle->SetOptStat("ourmen");

    myText_temp->SaveAs((PDFfileName + "[").c_str());
    const char* fileName = PDFfileName.c_str();
    // char fileName[PDFfileName.length()];
    // sprintf(fileName, "%s[", PDFfileName.c_str());
    // myText_temp->SaveAs(fileName);
    // sprintf(fileName, "%s", PDFfileName.c_str());

    for (int i = 0; i < HistoList.size(); i++) {
        myCanvas_temp->cd();

        myCanvas_temp->SetGrid();
        myCanvas_temp->SetBottomMargin(0.14), myCanvas_temp->SetLeftMargin(0.16), myCanvas_temp->SetRightMargin(0.23);
        myCanvas_temp->SetTopMargin(0.16);

        if (TH2* h2 = dynamic_cast<TH2*>(HistoList[i])) {
            h2->SetStats(kTRUE);

            h2->Draw("COLZ"), gPad->Modified(), gPad->Update();

            // Save current axis labels
            std::string xlab = h2->GetXaxis() ? h2->GetXaxis()->GetTitle() : "";
            std::string ylab = h2->GetYaxis() ? h2->GetYaxis()->GetTitle() : "";
            std::string zlab;

            if (auto* h = dynamic_cast<TH2*>(h2)) {
                if (h->GetZaxis()) { zlab = h->GetZaxis()->GetTitle(); }
            }

            // Convert title (and sanitize)
            const char* ct = h2->GetTitle();
            const std::string curr = ct ? std::string(ct) : std::string();
            const std::string converted = convert_title_to_splitline(curr);

            if (converted != curr) {
                h2->SetTitle(converted.c_str());  // Set only the title (no semicolons inside!)

                // Restore axis labels explicitly
                if (h2->GetXaxis()) h2->GetXaxis()->SetTitle(xlab.c_str());
                if (h2->GetYaxis()) h2->GetYaxis()->SetTitle(ylab.c_str());
                if (auto* h = dynamic_cast<TH2*>(h2)) {
                    if (h->GetZaxis()) { h->GetZaxis()->SetTitle(zlab.c_str()); }
                }

                if (auto* t = (TPaveText*)gPad->GetPrimitive("title")) { t->SetY1NDC(t->GetY1NDC() - 0.04), t->SetY2NDC(t->GetY2NDC() - 0.04), gPad->Modified(), gPad->Update(); }
            }

            // Stats box (guard for nullptr)
            TPaveStats* stats = dynamic_cast<TPaveStats*>(h2->FindObject("stats"));
            if (stats) { stats->SetX1NDC(0.78), stats->SetY1NDC(0.595), stats->SetX2NDC(0.98), stats->SetY2NDC(0.835); }

            // Palette (guard for nullptr)
            TPaletteAxis* palette = dynamic_cast<TPaletteAxis*>(h2->GetListOfFunctions()->FindObject("palette"));
            if (palette) { palette->SetX1NDC(0.775), palette->SetY1NDC(0.14), palette->SetX2NDC(0.82), palette->SetY2NDC(0.55); }

            if (hf::IsHistogramEmpty(h2)) { hf::DrawEmptyHistogramNotice(0.165, 0.4, 0.785, 0.6, 0.08); }

            gPad->Modified(), gPad->Update();
        } else if (TH1* h1 = dynamic_cast<TH1*>(HistoList[i])) {
            h1->SetStats(kTRUE);

            h1->Sumw2(), h1->Draw(), gPad->Modified(), gPad->Update();

            // Stats box (guard for nullptr)
            if (TPaveStats* stats = dynamic_cast<TPaveStats*>(h2->FindObject("stats"))) {
                stats->SetX1NDC(0.78), stats->SetY1NDC(0.595);
                stats->SetX2NDC(0.98), stats->SetY2NDC(0.835);
            }

            if (hf::IsHistogramEmpty(h1)) { hf::DrawEmptyHistogramNotice(0.165, 0.4, 0.785, 0.6, 0.08); }

            gPad->Modified(), gPad->Update();
        }

        myCanvas_temp->Print(fileName, "pdf");
        myCanvas_temp->Clear();
    }

    myCanvas_temp->Print((PDFfileName + "]").c_str(), "pdf");
    // sprintf(fileName, "%s]", PDFfileName.c_str());
    // myCanvas_temp->Print(fileName, "pdf");

    hf::FixPDFOrientation(PDFfileName);  // Fix orientation

    delete myText_temp;
    delete myCanvas_temp;
}
#pragma endregion

// DrawAndSaveMaps function ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* DrawAndSaveMaps function (G3) */
void AMaps::DrawAndSaveMaps(const std::string& SampleName, TCanvas* h1DCanvas, const std::string& AcceptanceMapsDirectory) {
    bool PrintOut = false;
    const bool hasElectronSample = bt::FindSubstring(SName, "Uniform_1e_sample_");
    const bool hasProtonSample = bt::FindSubstring(SName, "Uniform_ep_sample_");
    const bool hasNeutronSample = bt::FindSubstring(SName, "Uniform_en_sample_");

    if (PrintOut) { std::cout << "\n\nProcessing maps...\n"; }

    if (PrintOut) { std::cout << "\n\nGenerating maps directories...\n"; }
    system(("mkdir -p " + AcceptanceMapsDirectory + SampleName).c_str());

    if (PrintOut) { std::cout << "\n\nCalculating efficiencies...\n"; }
    CalcAcceptanceEfficiency();

    if (PrintOut) { std::cout << "\n\nGenerating filtered maps...\n"; }
    GenerateFilteredRecoMaps(Charged_particle_min_Ratio, Neutral_particle_min_Ratio);

    if (PrintOut) { std::cout << "\n\nGenerating map matrices...\n"; }
    GenerateMapMatrices(Charged_particle_min_Ratio, Neutral_particle_min_Ratio);
    GenerateExtendedElectronMapMatrices(e_AMap_Slices);           // First extension by e_AMap_Slices
    GenerateExtendedElectronMapMatrices(e_AMap_Slices_extended);  // Second extension by e_AMap_Slices_extended

    if (PrintOut) { std::cout << "\n\nSaving maps...\n"; }
    SaveMaps(SampleName, AcceptanceMapsDirectory);

    auto DrawSlices = [&](int numSlices, auto& truthBySlice, auto& recoBySlice, auto& ratioBySlice, auto& filteredBySlice) {
        for (int i = 0; i < numSlices; i++) {
            truthBySlice.at(i).hDrawAndSave(SName, h1DCanvas, TLAMaps, AMap_TL_HistoList, true);
            recoBySlice.at(i).hDrawAndSave(SName, h1DCanvas, RecoAMaps, AMap_Reco_HistoList, true);
            ratioBySlice.at(i).hDrawAndSave(SName, h1DCanvas, AMapsRatio, AMap_Ratio_HistoList, true);
            filteredBySlice.at(i).hDrawAndSave(SName, h1DCanvas, Charged_particle_Sep_AMaps, cPart_Sep_AMaps_HistoList, true);
        }
    };

    auto DrawConstNeutronSlices = [&](int numSlices) {
        for (int i = 0; i < numSlices; i++) {
            truth_theta_n_VS_phi_n_ByConstSlice.at(i).hDrawAndSave(SName, h1DCanvas, ConstTLAMaps, AMap_ConstTL_HistoList, true);
            reco_theta_n_VS_phi_n_ByConstSlice.at(i).hDrawAndSave(SName, h1DCanvas, ConstRecoAMaps, AMap_ConstReco_HistoList, true);
            acceptance_eff_n_ByConstSlice.at(i).hDrawAndSave(SName, h1DCanvas, ConstAMapsRatio, AMap_ConstRatio_HistoList, true);
            filtered_reco_theta_n_VS_phi_n_ByConstSlice.at(i).hDrawAndSave(SName, h1DCanvas, Const_neutral_particle_Sep_AMaps, nPart_Sep_AMaps_HistoList_Const, true);
        }
    };

    auto SaveSlicePDFs = [&](const std::vector<TObject*>& objects, const std::string& basePath, const std::string& fileName) { DrawAndSaveMapsBySliceToPDFs(objects, basePath + fileName); };

    if (hasElectronSample) {
        if (PrintOut) { std::cout << "\n\nPlotting electron maps...\n"; }

        /* Acceptance maps BC */
        reco_theta_e_VS_phi_e_BC.hDrawAndSave(SName, h1DCanvas, AcceptanceMapsBC, AMapsBC_HistoList, true);

        /* Electron maps */
        DrawSlices(ElectronMomSliceLimits.size(), truth_theta_e_VS_phi_e_BySlice, reco_theta_e_VS_phi_e_BySlice, acceptance_eff_e_BySlice, filtered_reco_theta_e_VS_phi_e_BySlice);

        /* Finalized acceptance maps */
        filtered_reco_theta_e_VS_phi_e.hDrawAndSave(SName, h1DCanvas, AcceptanceMaps, AMaps_HistoList, true);

        /* Saving maps in PDFs */
        SaveSlicePDFs(truth_e_BySlice, truth_theta_e_VS_phi_e_BySlice[0].GetHistogram2DSaveNamePath(), "truth_theta_e_VS_phi_e_BySlice.pdf");
        SaveSlicePDFs(reco_e_BySlice, reco_theta_e_VS_phi_e_BySlice[0].GetHistogram2DSaveNamePath(), "reco_theta_e_VS_phi_e_BySlice.pdf");
        SaveSlicePDFs(acc_eff_e_BySlice, acceptance_eff_e_BySlice[0].GetHistogram2DSaveNamePath(), "acceptance_eff_e_BySlice.pdf");
        SaveSlicePDFs(filtered_reco_e_BySlice, filtered_reco_theta_e_VS_phi_e_BySlice[0].GetHistogram2DSaveNamePath(), "filtered_reco_theta_e_VS_phi_e_BySlice.pdf");
    }

    if (hasProtonSample) {
        if (PrintOut) { std::cout << "\n\nPlotting proton maps...\n"; }

        /* Acceptance maps BC */
        reco_theta_p_VS_phi_p_BC.hDrawAndSave(SName, h1DCanvas, AcceptanceMapsBC, AMapsBC_HistoList, true);

        /* Proton maps */
        DrawSlices(NucleonMomSliceLimits.size(), truth_theta_p_VS_phi_p_BySlice, reco_theta_p_VS_phi_p_BySlice, acceptance_eff_p_BySlice, filtered_reco_theta_p_VS_phi_p_BySlice);

        /* Finalized acceptance maps */
        filtered_reco_theta_p_VS_phi_p.hDrawAndSave(SName, h1DCanvas, AcceptanceMaps, AMaps_HistoList, true);

        /* Saving maps in PDFs */
        SaveSlicePDFs(truth_p_BySlice, truth_theta_p_VS_phi_p_BySlice[0].GetHistogram2DSaveNamePath(), "truth_theta_p_VS_phi_p_BySlice.pdf");
        SaveSlicePDFs(reco_p_BySlice, reco_theta_p_VS_phi_p_BySlice[0].GetHistogram2DSaveNamePath(), "reco_theta_p_VS_phi_p_BySlice.pdf");
        SaveSlicePDFs(acc_eff_p_BySlice, acceptance_eff_p_BySlice[0].GetHistogram2DSaveNamePath(), "acceptance_eff_p_BySlice.pdf");
        SaveSlicePDFs(filtered_reco_p_BySlice, filtered_reco_theta_p_VS_phi_p_BySlice[0].GetHistogram2DSaveNamePath(), "filtered_reco_theta_p_VS_phi_p_BySlice.pdf");
    }

    if (hasNeutronSample) {
        if (PrintOut) { std::cout << "\n\nPlotting proton maps...\n"; }

        /* Acceptance maps BC */
        reco_theta_n_VS_phi_n_BC.hDrawAndSave(SName, h1DCanvas, AcceptanceMapsBC, AMapsBC_HistoList, true);

        /* Neutron maps (by momentum slice) */
        DrawSlices(NucleonMomSliceLimits.size(), truth_theta_n_VS_phi_n_BySlice, reco_theta_n_VS_phi_n_BySlice, acceptance_eff_n_BySlice, filtered_reco_theta_n_VS_phi_n_BySlice);
        DrawConstNeutronSlices(NucleonMomSliceLimits.size());

        /* Neutron maps */
        truth_theta_n_VS_phi_n.hDrawAndSave(SName, h1DCanvas, TLAMaps, AMap_TL_HistoList, true);
        reco_theta_n_VS_phi_n.hDrawAndSave(SName, h1DCanvas, RecoAMaps, AMap_Reco_HistoList, true);
        acceptance_eff_n.hDrawAndSave(SName, h1DCanvas, AMapsRatio, AMap_Ratio_HistoList, true);

        /* Finalized acceptance maps */
        filtered_reco_theta_n_VS_phi_n.hDrawAndSave(SName, h1DCanvas, AcceptanceMaps, AMaps_HistoList, true);

        /* Saving maps in PDFs */
        SaveSlicePDFs(truth_n_BySlice, truth_theta_n_VS_phi_n_BySlice[0].GetHistogram2DSaveNamePath(), "truth_theta_n_VS_phi_n_BySlice.pdf");
        SaveSlicePDFs(reco_n_BySlice, reco_theta_n_VS_phi_n_BySlice[0].GetHistogram2DSaveNamePath(), "reco_theta_n_VS_phi_n_BySlice.pdf");
        SaveSlicePDFs(acc_eff_n_BySlice, acceptance_eff_n_BySlice[0].GetHistogram2DSaveNamePath(), "acceptance_eff_n_BySlice.pdf");
        SaveSlicePDFs(filtered_reco_n_BySlice, filtered_reco_theta_n_VS_phi_n_BySlice[0].GetHistogram2DSaveNamePath(), "filtered_reco_theta_n_VS_phi_n_BySlice.pdf");

        /* Saving maps in PDFs (const) */
        SaveSlicePDFs(truth_n_ByConstSlice, truth_theta_n_VS_phi_n_ByConstSlice[0].GetHistogram2DSaveNamePath(), "truth_theta_n_VS_phi_n_ByConstSlice.pdf");
        SaveSlicePDFs(reco_n_ByConstSlice, reco_theta_n_VS_phi_n_ByConstSlice[0].GetHistogram2DSaveNamePath(), "reco_theta_n_VS_phi_n_ByConstSlice.pdf");
        SaveSlicePDFs(acc_eff_n_ByConstSlice, acceptance_eff_n_ByConstSlice[0].GetHistogram2DSaveNamePath(), "acceptance_eff_n_ByConstSlice.pdf");
        SaveSlicePDFs(filtered_reco_n_ByConstSlice, filtered_reco_theta_n_VS_phi_n_ByConstSlice[0].GetHistogram2DSaveNamePath(), "filtered_reco_theta_n_VS_phi_n_ByConstSlice.pdf");
    }

    // A helper lambda that saves acceptance maps into TFiles
    auto SaveAMapsToROOTFile = [&SampleName](const std::string SavePath, const std::string AMaps_prefix, TList* AcceptanceMapsList, const bool& CopyAMapsToCopyPath,
                                             const std::string AMapsCopySavePath) {
        std::string ROOTFileName = AMaps_prefix + SampleName + ".root";
        std::string ROOTFile = SavePath + "/" + ROOTFileName;

        // if (gSystem->mkdir(refDir.c_str(), kTRUE) != 0) {
        //     // mkdir returns 0 on success; allow "already exists". Verify access:
        void* dirp = gSystem->OpenDirectory(SavePath.c_str());

        if (!dirp) {
            std::cerr << "AMaps::DrawAndSaveMaps: ERROR: cannot access '" << SavePath << "'\n";
            return;  // or throw
        }
        gSystem->FreeDirectory(dirp);
        // }

        TFile* AMapsROOTFile = new TFile(ROOTFile.c_str(), "recreate");
        if (!AMapsROOTFile || AMapsROOTFile->IsZombie()) {
            bt::ExitWithError(__func__, __FILE__, __LINE__, "ERROR opening '" + ROOTFile + "'");
            // return nullptr;  // or throw
        }

        AMapsROOTFile->cd();
        AcceptanceMapsList->Write();
        AMapsROOTFile->Write();
        AMapsROOTFile->Close();

        if (CopyAMapsToCopyPath) { system(("cp " + ROOTFile + " " + AMapsCopySavePath).c_str()); }
    };

#pragma region /* Saving acceptance maps to ROOT files */
    if (PrintOut) { std::cout << "\n\nSaving acceptance maps to ROOT files...\n\n"; }

    SaveAMapsToROOTFile(AMapSavePath, AMapsBC_prefix, AcceptanceMapsBC, true, AMapCopySavePath);                    // Acceptance maps BC
    SaveAMapsToROOTFile(AMapSavePath, AMap_TL_prefix, TLAMaps, true, AMapCopySavePath);                             // TL Acceptance maps
    SaveAMapsToROOTFile(AMapSavePath, AMap_Reco_prefix, RecoAMaps, true, AMapCopySavePath);                         // Reco Acceptance maps
    SaveAMapsToROOTFile(AMapSavePath, AMap_Ratio_prefix, AMapsRatio, true, AMapCopySavePath);                       // Ratio Acceptance maps
    SaveAMapsToROOTFile(AMapSavePath, cPart_Sep_AMaps_prefix, Charged_particle_Sep_AMaps, true, AMapCopySavePath);  // Charged particle separate AMaps
    SaveAMapsToROOTFile(AMapSavePath, AMaps_prefix, AcceptanceMaps, true, AMapCopySavePath);                        // Acceptance maps

    SaveAMapsToROOTFile(AMapSavePath, AMap_TL_prefix + "_const_", ConstTLAMaps, true, AMapCopySavePath);                              // TL Acceptance maps
    SaveAMapsToROOTFile(AMapSavePath, AMap_Reco_prefix + "_const_", ConstRecoAMaps, true, AMapCopySavePath);                          // Reco Acceptance maps
    SaveAMapsToROOTFile(AMapSavePath, AMap_Ratio_prefix + "_const_", ConstAMapsRatio, true, AMapCopySavePath);                        // Ratio Acceptance maps
    SaveAMapsToROOTFile(AMapSavePath, nPart_Sep_AMaps_prefix + "_const_", Const_neutral_particle_Sep_AMaps, true, AMapCopySavePath);  // Charged particle separate AMaps
#pragma endregion

    if (PrintOut) { std::cout << "done!\n\n\n"; }
}
#pragma endregion

// HistCounter function -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* HistCounter function */
int AMaps::HistCounter(const char* fname) {
    bool PrintHistInfo = false;

    TKey* key;
    TFile* f = TFile::Open(fname, "READ");

    Int_t total = 0;
    TIter next((TList*)f->GetListOfKeys());

    while ((key = (TKey*)next())) {
        TClass* cl = gROOT->GetClass(key->GetClassName());

        if (cl->InheritsFrom("TH1")) {
            // the following line is not needed if you only want
            // to count the histograms
            TH1* h = (TH1*)key->ReadObj();

            if (PrintHistInfo) { std::cout << "Histo found: " << h->GetName() << " - " << h->GetTitle() << endl; }

            total++;
        }
    }

    if (PrintHistInfo) { std::cout << "\n\nFound " << total << " Histograms\n" << endl; }

    return total;
}
#pragma endregion

// SetHistBinsFromHistTitle function ------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* SetHistBinsFromHistTitle function */
void AMaps::SetHistBinsFromHistTitle(TH2D* Histogram2D) {
    bool PrintOut = false;

    std::string Title = Histogram2D->GetTitle();
    std::string NumOfXBinsStr = Title.substr((Title.find_last_of('(')) + 1, 3);
    std::string NumOfYBinsStr = Title.substr(Title.find_last_of('x') + 1, 3);

    HistNucSliceNumOfXBins = stoi(NumOfXBinsStr);
    HistNucSliceNumOfYBins = stoi(NumOfYBinsStr);

    if (PrintOut) {
        std::cout << "\n\nTitle = " << Title << "\n\n";
        std::cout << "NumOfXBinsStr = " << NumOfXBinsStr << "\n";
        std::cout << "NumOfYBinsStr = " << NumOfYBinsStr << "\n\n";

        std::cout << "HistNucSliceNumOfXBins = " << HistNucSliceNumOfXBins << "\n";
        std::cout << "HistNucSliceNumOfYBins = " << HistNucSliceNumOfYBins << "\n\n";
    }
}
#pragma endregion

// SetSlicesFromHistTitle function --------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* SetSlicesFromHistTitle function */
void AMaps::SetSlicesFromHistTitle(TH2D* Histogram2D, const std::string& Particle) {
    bool PrintOut = false;

    std::string Title = Histogram2D->GetTitle();
    std::string SliceLowerLimStr = Title.substr((Title.find_first_of('#')) - 4, 4);
    std::string SliceUpperLimStr = Title.substr(Title.find_last_of('#') + 4, 4);

    double SliceLowerLim = stod(SliceLowerLimStr);
    double SliceUpperLim = stod(SliceUpperLimStr);

    std::vector<double> MomBinsLimitsTemp = {SliceLowerLim, SliceUpperLim};

    if (isElectron(Particle)) {
        ElectronMomSliceLimits.push_back(MomBinsLimitsTemp);
    } else if (isProton(Particle) || isNeutron(Particle)) {
        NucleonMomSliceLimits.push_back(MomBinsLimitsTemp);
    }

    if (PrintOut) {
        std::cout << "\n\nTitle = " << Title << "\n\n";
        std::cout << "SliceLowerLimStr = " << SliceLowerLimStr << "\n";
        std::cout << "SliceUpperLimStr = " << SliceUpperLimStr << "\n\n";

        std::cout << "SliceLowerLim = " << SliceLowerLim << "\n";
        std::cout << "SliceUpperLim = " << SliceUpperLim << "\n\n";
    }
}
#pragma endregion

#pragma region /* SetSlicesFromHistTitle function (original) */
void AMaps::SetSlicesFromHistTitle(TH2D* Histogram2D, std::vector<std::vector<double>> MomBinsLimits) {
    bool PrintOut = false;

    std::string Title = Histogram2D->GetTitle();
    std::string SliceLowerLimStr = Title.substr((Title.find_first_of('#')) - 4, 4);
    std::string SliceUpperLimStr = Title.substr(Title.find_last_of('#') + 4, 4);

    double SliceLowerLim = stod(SliceLowerLimStr);
    double SliceUpperLim = stod(SliceUpperLimStr);

    std::vector<double> MomBinsLimitsTemp = {SliceLowerLim, SliceUpperLim};
    NucleonMomSliceLimits.push_back(MomBinsLimitsTemp);

    if (PrintOut) {
        std::cout << "\n\nTitle = " << Title << "\n\n";
        std::cout << "SliceLowerLimStr = " << SliceLowerLimStr << "\n";
        std::cout << "SliceUpperLimStr = " << SliceUpperLimStr << "\n\n";

        std::cout << "SliceLowerLim = " << SliceLowerLim << "\n";
        std::cout << "SliceUpperLim = " << SliceUpperLim << "\n\n";

        std::cout << "MomBinsLimitsTemp.size() = " << MomBinsLimitsTemp.size() << "\n\n";
    }
}
#pragma endregion

// ReadHitMaps function -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* ReadHitMaps function */
void AMaps::ReadHitMaps(const std::string& AcceptanceMapsDirectory, const std::string& SampleName) {
    bool PrintKeys = false;

#pragma region /* Load AMapsBC */
    std::string AMapsBC_RootFile_FileName = AcceptanceMapsDirectory + "/" + SampleName + "/" + AMapsBC_prefix + SampleName + ".root";
    TFile* AMapsBC_RootFile = new TFile(AMapsBC_RootFile_FileName.c_str());
    if (!AMapsBC_RootFile) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load AMapsBC root file!"); }

    for (TObject* keyAsObj : *AMapsBC_RootFile->GetListOfKeys()) {
        auto key = dynamic_cast<TKey*>(keyAsObj);

        if (PrintKeys) { std::cout << "Key name: " << key->GetName() << " Type: " << key->GetClassName() << endl; }

        TH2D* TempHist = (TH2D*)keyAsObj;

        if (bt::FindSubstring(TempHist->GetTitle(), "Electron") || bt::FindSubstring(TempHist->GetTitle(), "electron")) {
            reco_theta_e_VS_phi_e_BC.SetHistogram2D(TempHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "Proton") || bt::FindSubstring(TempHist->GetTitle(), "proton")) {
            reco_theta_p_VS_phi_p_BC.SetHistogram2D(TempHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "Neutron") || bt::FindSubstring(TempHist->GetTitle(), "neutron")) {
            reco_theta_n_VS_phi_n_BC.SetHistogram2D(TempHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "Nucleon") || bt::FindSubstring(TempHist->GetTitle(), "nucleon")) {
            reco_theta_nuc_VS_phi_nuc_BC.SetHistogram2D(TempHist);
        }
    }
#pragma endregion

#pragma region /* Load Hit_Maps_TL */
    std::string Hit_Maps_TL_RootFile_FileName = AcceptanceMapsDirectory + "/" + SampleName + "/" + AMap_TL_prefix + SampleName + ".root";
    TFile* Hit_Maps_TL_RootFile = new TFile(Hit_Maps_TL_RootFile_FileName.c_str());
    if (!Hit_Maps_TL_RootFile) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Hit_Maps_TL root file!"); }

    int counter = 0;

    for (TObject* keyAsObj : *Hit_Maps_TL_RootFile->GetListOfKeys()) {
        auto key = dynamic_cast<TKey*>(keyAsObj);

        if (PrintKeys) { std::cout << "Key name: " << key->GetName() << " Type: " << key->GetClassName() << endl; }

        TH2D* TempHist = (TH2D*)keyAsObj;
        hPlot2D Temp2DHist;
        Temp2DHist.SetHistogram2D(TempHist);

        if (counter == 0) { SetHistBinsFromHistTitle(TempHist); }

        if (bt::FindSubstring(TempHist->GetTitle(), "{" + eLabel + "}")) {
            truth_theta_e_VS_phi_e_BySlice.push_back(Temp2DHist);
            SetSlicesFromHistTitle(TempHist, NucleonMomSliceLimits);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "{" + pLabel + "}")) {
            truth_theta_p_VS_phi_p_BySlice.push_back(Temp2DHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "{" + nLabel + "}")) {
            truth_theta_n_VS_phi_n.SetHistogram2D(TempHist);
        }

        ++counter;
    }
#pragma endregion

#pragma region /* Load Hit_Maps_Reco */
    std::string Hit_Maps_Reco_RootFile_FileName = AcceptanceMapsDirectory + "/" + SampleName + "/" + AMap_Reco_prefix + SampleName + ".root";
    TFile* Hit_Maps_Reco_RootFile = new TFile(Hit_Maps_Reco_RootFile_FileName.c_str());
    if (!Hit_Maps_Reco_RootFile) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Hit_Maps_Reco root file!"); }

    for (TObject* keyAsObj : *Hit_Maps_Reco_RootFile->GetListOfKeys()) {
        auto key = dynamic_cast<TKey*>(keyAsObj);

        if (PrintKeys) { std::cout << "Key name: " << key->GetName() << " Type: " << key->GetClassName() << endl; }

        TH2D* TempHist = (TH2D*)keyAsObj;
        hPlot2D Temp2DHist;
        Temp2DHist.SetHistogram2D(TempHist);

        if (bt::FindSubstring(TempHist->GetTitle(), "{" + eLabel + "}")) {
            reco_theta_e_VS_phi_e_BySlice.push_back(Temp2DHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "{" + pLabel + "}")) {
            reco_theta_p_VS_phi_p_BySlice.push_back(Temp2DHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "{" + nLabel + "}")) {
            reco_theta_n_VS_phi_n.SetHistogram2D(TempHist);
        }
    }
#pragma endregion

#pragma region /* Load Hit_Maps_Ratio */
    std::string Hit_Maps_Ratio_RootFile_FileName = AcceptanceMapsDirectory + "/" + SampleName + "/" + AMap_Ratio_prefix + SampleName + ".root";
    TFile* Hit_Maps_Ratio_RootFile = new TFile(Hit_Maps_Ratio_RootFile_FileName.c_str());
    if (!Hit_Maps_Ratio_RootFile) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Hit_Maps_Ratio root file!"); }

    for (TObject* keyAsObj : *Hit_Maps_Ratio_RootFile->GetListOfKeys()) {
        auto key = dynamic_cast<TKey*>(keyAsObj);

        if (PrintKeys) { std::cout << "Key name: " << key->GetName() << " Type: " << key->GetClassName() << endl; }

        TH2D* TempHist = (TH2D*)keyAsObj;
        hPlot2D Temp2DHist;
        Temp2DHist.SetHistogram2D(TempHist);

        if (bt::FindSubstring(TempHist->GetTitle(), "{" + eLabel + "}")) {
            acceptance_eff_e_BySlice.push_back(Temp2DHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "{" + pLabel + "}")) {
            acceptance_eff_p_BySlice.push_back(Temp2DHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "{" + nLabel + "}")) {
            acceptance_eff_n.SetHistogram2D(TempHist);
        }
    }
#pragma endregion

#pragma region /* Load cPart_Sep_AMaps */
    std::string cPart_Sep_AMaps_RootFile_FileName = AcceptanceMapsDirectory + "/" + SampleName + "/" + cPart_Sep_AMaps_prefix + SampleName + ".root";
    TFile* cPart_Sep_AMaps_RootFile = new TFile(cPart_Sep_AMaps_RootFile_FileName.c_str());
    if (!cPart_Sep_AMaps_RootFile) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load cPart_Sep_AMaps root file!"); }

    for (TObject* keyAsObj : *cPart_Sep_AMaps_RootFile->GetListOfKeys()) {
        auto key = dynamic_cast<TKey*>(keyAsObj);

        if (PrintKeys) { std::cout << "Key name: " << key->GetName() << " Type: " << key->GetClassName() << endl; }

        TH2D* TempHist = (TH2D*)keyAsObj;
        hPlot2D Temp2DHist;
        Temp2DHist.SetHistogram2D(TempHist);

        if (bt::FindSubstring(TempHist->GetTitle(), "Electron") || bt::FindSubstring(TempHist->GetTitle(), "electron")) {
            filtered_reco_theta_e_VS_phi_e_BySlice.push_back(Temp2DHist);
        } else if (bt::FindSubstring(TempHist->GetTitle(), "Proton") || bt::FindSubstring(TempHist->GetTitle(), "proton")) {
            filtered_reco_theta_p_VS_phi_p_BySlice.push_back(Temp2DHist);
        }
    }
#pragma endregion

#pragma region /* Load AMaps */
    std::string AMaps_RootFile_FileName = AcceptanceMapsDirectory + "/" + SampleName + "/" + AMaps_prefix + SampleName + ".root";
    TFile* AMaps_RootFile = new TFile(AMaps_RootFile_FileName.c_str());
    if (!AMaps_RootFile) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load AMaps root file!"); }

    LoadedElectronAMaps0 = (TH2D*)AMaps_RootFile->Get("Electron_AMap");
    if (!LoadedElectronAMaps0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Electron_AMap from root file!"); }

    LoadedProtonAMap = (TH2D*)AMaps_RootFile->Get("Proton_AMap");
    if (!LoadedProtonAMap) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Proton_AMap from root file!"); }

    LoadedNeutronAMap = (TH2D*)AMaps_RootFile->Get("Neutron_AMap");
    if (!LoadedNeutronAMap) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Neutron_AMap from root file!"); }

    LoadedNucleonAMap = (TH2D*)AMaps_RootFile->Get("Nucleon_AMap");
    if (!LoadedNucleonAMap) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load Nucleon_AMap from root file!"); }
#pragma endregion

    std::cout << "\n\nAcceptance maps loaded!\n\n";
}
#pragma endregion

// LoadWMapsFromRatioROOTFiles function -----------------------------------------------------------------------------------------------------------------------------

#pragma region /* LoadWMapsFromRatioROOTFiles function */
void AMaps::LoadWMapsFromRatioROOTFiles(const std::string& SampleName, const std::string& InputAcceptanceMapsDirectory, const std::string& OutputAcceptanceMapsDirectory) {
    auto ensureTrailingSlash = [](std::string path) {
        if (!path.empty() && path.back() != '/') { path.push_back('/'); }
        return path;
    };

    const std::string inputDir = ensureTrailingSlash(InputAcceptanceMapsDirectory);
    const std::string outputDir = ensureTrailingSlash(OutputAcceptanceMapsDirectory.empty() ? InputAcceptanceMapsDirectory : OutputAcceptanceMapsDirectory);

    struct SliceHistogram {
        double lowerLimit = 0.0;
        TH2D* histogram = nullptr;
    };

    auto histogramToWMap = [](TH2D* hist) {
        std::vector<std::vector<double>> wmap;
        if (!hist) { return wmap; }

        const int xBins = hist->GetNbinsX();
        const int yBins = hist->GetNbinsY();
        wmap.reserve(yBins);

        for (int y = 0; y < yBins; y++) {
            std::vector<double> row;
            row.reserve(xBins);

            for (int x = 0; x < xBins; x++) { row.push_back(hist->GetBinContent(x + 1, y + 1)); }

            wmap.push_back(row);
        }

        return wmap;
    };

    auto extractSliceLowerLimit = [](const std::string& histName) {
        const std::string fromTag = "_from_";
        const std::string toTag = "_to_";
        const size_t fromPos = histName.find(fromTag);
        const size_t toPos = histName.find(toTag);

        if (fromPos == std::string::npos || toPos == std::string::npos || toPos <= (fromPos + fromTag.size())) { return -1e9; }

        const std::string value = histName.substr(fromPos + fromTag.size(), toPos - (fromPos + fromTag.size()));
        try {
            return std::stod(value);
        } catch (...) { return -1e9; }
    };

    auto readRatioFile = [&](const std::string& rootFilePath, bool isConstFile, std::vector<SliceHistogram>& electronSlices, std::vector<SliceHistogram>& protonSlices,
                             std::vector<SliceHistogram>& neutronSlices, std::vector<SliceHistogram>& neutronConstSlices, TH2D*& neutronCombined) {
        auto consumeHistogram = [&](TH2D* hist) {
            if (!hist) { return; }

            hist->SetDirectory(nullptr);
            const std::string title = hist->GetTitle();
            const std::string name = hist->GetName();
            const bool isSlice = (name.find("_for_P_from_") != std::string::npos) || (title.find("P^{truth}_{") != std::string::npos);

            if (title.find("{" + eLabel + "}") != std::string::npos) {
                electronSlices.push_back({extractSliceLowerLimit(name), hist});
            } else if (title.find("{" + pLabel + "}") != std::string::npos) {
                protonSlices.push_back({extractSliceLowerLimit(name), hist});
            } else if (title.find("{" + nLabel + "}") != std::string::npos || title.find("Neutron") != std::string::npos || title.find("neutron") != std::string::npos) {
                if (!isSlice) {
                    neutronCombined = hist;
                } else if (isConstFile) {
                    neutronConstSlices.push_back({extractSliceLowerLimit(name), hist});
                } else {
                    neutronSlices.push_back({extractSliceLowerLimit(name), hist});
                }
            }
        };

        TFile* rootFile = TFile::Open(rootFilePath.c_str(), "READ");
        if (!rootFile || rootFile->IsZombie()) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Could not load ratio root file: " + rootFilePath); }

        TIter nextKey(rootFile->GetListOfKeys());
        TKey* key = nullptr;

        while ((key = (TKey*)nextKey())) {
            TObject* obj = key->ReadObj();
            if (!obj) { continue; }

            if (obj->InheritsFrom(TH2D::Class())) {
                consumeHistogram(dynamic_cast<TH2D*>(obj));
            } else if (obj->InheritsFrom(TList::Class())) {
                auto* list = dynamic_cast<TList*>(obj);
                if (!list) { continue; }

                TIter nextObj(list);
                TObject* listObj = nullptr;
                while ((listObj = nextObj())) {
                    if (!listObj->InheritsFrom(TH2D::Class())) { continue; }
                    TH2D* hist = dynamic_cast<TH2D*>(listObj);
                    if (!hist) { continue; }
                    TH2D* clonedHist = dynamic_cast<TH2D*>(hist->Clone(hist->GetName()));
                    consumeHistogram(clonedHist);
                }
            }
        }

        rootFile->Close();
    };

    auto sortBySliceLowerLimit = [](std::vector<SliceHistogram>& slices) {
        std::sort(slices.begin(), slices.end(), [](const SliceHistogram& a, const SliceHistogram& b) { return a.lowerLimit < b.lowerLimit; });
    };

    e_WMap_Slices.clear();
    p_WMap_Slices.clear();
    n_WMap.clear();
    n_WMap_Slices.clear();
    n_WMap_ConstSlices.clear();

    const std::string ratioRootFile = inputDir + SampleName + "/" + AMap_Ratio_prefix + SampleName + ".root";
    const std::string constRatioRootFile = inputDir + SampleName + "/" + AMap_Ratio_prefix + "_const_" + SampleName + ".root";

    std::vector<SliceHistogram> electronSlices;
    std::vector<SliceHistogram> protonSlices;
    std::vector<SliceHistogram> neutronSlices;
    std::vector<SliceHistogram> neutronConstSlices;
    TH2D* neutronCombined = nullptr;

    readRatioFile(ratioRootFile, false, electronSlices, protonSlices, neutronSlices, neutronConstSlices, neutronCombined);

    TFile* constRatioFileProbe = TFile::Open(constRatioRootFile.c_str(), "READ");
    if (constRatioFileProbe && !constRatioFileProbe->IsZombie()) {
        constRatioFileProbe->Close();
        readRatioFile(constRatioRootFile, true, electronSlices, protonSlices, neutronSlices, neutronConstSlices, neutronCombined);
    } else if (constRatioFileProbe) {
        constRatioFileProbe->Close();
    }

    sortBySliceLowerLimit(electronSlices);
    sortBySliceLowerLimit(protonSlices);
    sortBySliceLowerLimit(neutronSlices);
    sortBySliceLowerLimit(neutronConstSlices);

    for (const auto& slice : electronSlices) { e_WMap_Slices.push_back(histogramToWMap(slice.histogram)); }
    for (const auto& slice : protonSlices) { p_WMap_Slices.push_back(histogramToWMap(slice.histogram)); }
    for (const auto& slice : neutronSlices) { n_WMap_Slices.push_back(histogramToWMap(slice.histogram)); }
    for (const auto& slice : neutronConstSlices) { n_WMap_ConstSlices.push_back(histogramToWMap(slice.histogram)); }
    n_WMap = histogramToWMap(neutronCombined);

    auto readSliceLimitsIfExists = [&](const std::string& limitsFilePath, std::vector<std::vector<double>>& destination) {
        std::ifstream probe(limitsFilePath);
        if (!probe.good()) { return; }
        probe.close();

        destination.clear();
        ReadAMapLimits(limitsFilePath.c_str(), destination);
    };

    if (ElectronMomSliceLimits.empty() && !e_WMap_Slices.empty()) {
        const std::string eSliceLimitsFile = inputDir + SampleName + "/e_AMaps_by_slice/e_slice_limits.par";
        readSliceLimitsIfExists(eSliceLimitsFile, ElectronMomSliceLimits);
    }

    if (NucleonMomSliceLimits.empty() && (!p_WMap_Slices.empty() || !n_WMap_Slices.empty() || !n_WMap_ConstSlices.empty())) {
        const std::string pSliceLimitsFile = inputDir + SampleName + "/p_AMaps_by_slice/p_slice_limits.par";
        const std::string nSliceLimitsFile = inputDir + SampleName + "/n_AMaps_by_slice/n_slice_limits.par";
        readSliceLimitsIfExists(pSliceLimitsFile, NucleonMomSliceLimits);
        if (NucleonMomSliceLimits.empty()) { readSliceLimitsIfExists(nSliceLimitsFile, NucleonMomSliceLimits); }
    }

    if (!e_WMap_Slices.empty() && !e_WMap_Slices.at(0).empty() && !e_WMap_Slices.at(0).at(0).empty()) {
        HistElectronSliceNumOfYBins = e_WMap_Slices.at(0).size();
        HistElectronSliceNumOfXBins = e_WMap_Slices.at(0).at(0).size();
    }

    if (!n_WMap_Slices.empty() && !n_WMap_Slices.at(0).empty() && !n_WMap_Slices.at(0).at(0).empty()) {
        HistNucSliceNumOfYBins = n_WMap_Slices.at(0).size();
        HistNucSliceNumOfXBins = n_WMap_Slices.at(0).at(0).size();
    } else if (!p_WMap_Slices.empty() && !p_WMap_Slices.at(0).empty() && !p_WMap_Slices.at(0).at(0).empty()) {
        HistNucSliceNumOfYBins = p_WMap_Slices.at(0).size();
        HistNucSliceNumOfXBins = p_WMap_Slices.at(0).at(0).size();
    } else if (!n_WMap_ConstSlices.empty() && !n_WMap_ConstSlices.at(0).empty() && !n_WMap_ConstSlices.at(0).at(0).empty()) {
        HistNucSliceNumOfYBins = n_WMap_ConstSlices.at(0).size();
        HistNucSliceNumOfXBins = n_WMap_ConstSlices.at(0).at(0).size();
    }

    SName = SampleName;
    AMapCopySavePath = outputDir + SampleName;
    system(("mkdir -p " + AMapCopySavePath).c_str());

    const std::string modeBackup = Maps_Mode;
    Maps_Mode = "WMaps";
    SaveMaps(SampleName, outputDir);
    Maps_Mode = modeBackup;
}
#pragma endregion

// ReadAMapLimits function ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* ReadAMapLimits function */
void AMaps::ReadAMapLimits(const char* filename, std::vector<std::vector<double>>& Loaded_particle_limits) {
    ifstream infile;
    infile.open(filename);

    if (infile.is_open()) {
        std::string tp;

        // getline(infile, tp) = read data from file object and put it into string.
        while (getline(infile, tp)) {
            stringstream ss(tp);
            std::string parameter, parameter2;
            ss >> parameter;  // get cut identifier

            if (bt::FindSubstring(parameter, "_slice_")) {
                // get cut values
                ss >> parameter2;
                stringstream ss2(parameter2);

                std::string LineEntry;
                std::vector<double> particle_limits;

                while (getline(ss2, LineEntry, ':')) { particle_limits.push_back(stod(LineEntry)); }

                Loaded_particle_limits.push_back(particle_limits);
            }
        }
    } else {
        bt::ExitWithError(__func__, __FILE__, __LINE__, std::string("File not found! Target file was set to:\n") + filename);
    }
}
#pragma endregion

// ReadMapSlices function (AMaps/WMaps) ---------------------------------------------------------------------------------------------------------------------------------

#pragma region /* ReadMapSlices function (AMaps/WMaps) */
void AMaps::ReadMapSlices(const std::string& SampleName, const std::string& AcceptanceMapsDirectory, const std::string& Particle,
                          const std::vector<std::vector<double>>& Loaded_particle_limits, std::vector<std::vector<std::vector<int>>>* Loaded_Particle_AMap_Slices,
                          std::vector<std::vector<std::vector<double>>>* Loaded_Particle_WMap_Slices, const bool Special_maps) {
    bool PrintOut = true;

    const bool isAMapsMode = (Maps_Mode == "AMaps");
    const bool isWMapsMode = (Maps_Mode == "WMaps");

    if (!isAMapsMode && !isWMapsMode) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Unsupported Maps_Mode: " + Maps_Mode); }
    if (isAMapsMode && !Loaded_Particle_AMap_Slices) { bt::ExitWithError(__func__, __FILE__, __LINE__, "AMaps destination container is null"); }
    if (isWMapsMode && !Loaded_Particle_WMap_Slices) { bt::ExitWithError(__func__, __FILE__, __LINE__, "WMaps destination container is null"); }

    const std::string ParticleShort = isElectron(Particle) ? "e" : (isProton(Particle) ? "p" : (isNeutron(Particle) ? "n" : "nuc"));
    const std::string ExtendedMaps = (isElectron(Particle) && Special_maps) ? "_extended" : "";

    for (int Slice = 0; Slice < Loaded_particle_limits.size(); Slice++) {
        const std::string lowLim = bt::ToStringWithPrecision(Loaded_particle_limits.at(Slice).at(0), 2);
        const std::string highLim = bt::ToStringWithPrecision(Loaded_particle_limits.at(Slice).at(1), 2);
        const std::string basePath = AcceptanceMapsDirectory + SampleName + "/";
        std::string fileToRead;
        std::vector<std::string> candidateFiles;

        if (isAMapsMode) {
            if (isNeutron(Particle) && Special_maps) {
                candidateFiles.push_back(ParticleShort + "_" + Maps_Mode + "_by_ConstSlice/" + ParticleShort + "_" + Maps_Mode + "_file_from_" + lowLim + "_to_" + highLim + ".par");
                candidateFiles.push_back(ParticleShort + "_AMap_by_ConstSlice/" + ParticleShort + "_AMap_file_from_" + lowLim + "_to_" + highLim + ".par");
            } else {
                candidateFiles.push_back(ParticleShort + ExtendedMaps + "_" + Maps_Mode + "_by_slice/" + ParticleShort + "_" + Maps_Mode + "_file_from_" + lowLim + "_to_" + highLim +
                                         ".par");
                candidateFiles.push_back(ParticleShort + ExtendedMaps + "_AMap_by_slice/" + ParticleShort + "_AMap_file_from_" + lowLim + "_to_" + highLim + ".par");
            }
        } else {
            const std::string primaryFileName = ParticleShort + "_" + Maps_Mode + "_by_slice/" + ParticleShort + "_" + Maps_Mode + "_file_from_" + lowLim + "_to_" + highLim + ".par";
            const std::string legacyFileName = ParticleShort + "_WMap_by_slice/" + ParticleShort + "_WMap_file_from_" + lowLim + "_to_" + highLim + ".par";
            candidateFiles.push_back(primaryFileName);
            candidateFiles.push_back(legacyFileName);
        }

        fileToRead = candidateFiles.front();
        for (const auto& candidate : candidateFiles) {
            if (!bt::fileExists((basePath + candidate).c_str())) { continue; }
            fileToRead = candidate;
            break;
        }

        if (PrintOut) {
            std::cout << "\033[32m" << "Reading " << Particle << ((Special_maps && particleType == ELECTRON_TYPE) ? " extended " : " ") << Maps_Mode << " from " << "\033[0m" << lowLim
                      << "\033[32m" << " to " << "\033[0m" << highLim << "\033[32m" << " [GeV/c]:" << "\033[0m" << " " << fileToRead << "\n";
        }

        if (isAMapsMode) {
            std::vector<std::vector<int>> Loaded_Particle_AMap_TempSlice;
            ReadMap((basePath + fileToRead).c_str(), &Loaded_Particle_AMap_TempSlice);
            Loaded_Particle_AMap_Slices->push_back(Loaded_Particle_AMap_TempSlice);
        } else {
            std::vector<std::vector<double>> Loaded_Particle_WMap_TempSlice;
            ReadMap((basePath + fileToRead).c_str(), nullptr, &Loaded_Particle_WMap_TempSlice);
            Loaded_Particle_WMap_Slices->push_back(Loaded_Particle_WMap_TempSlice);
        }
    }
}
#pragma endregion

// ReadMap function (AMaps/WMaps) ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* ReadMap function (AMaps/WMaps) */
void AMaps::ReadMap(const char* filename, std::vector<std::vector<int>>* Loaded_particle_AMap, std::vector<std::vector<double>>* Loaded_particle_WMap) {
    const bool PrintOut = false;
    const bool isAMapsMode = (Maps_Mode == "AMaps");
    const bool isWMapsMode = (Maps_Mode == "WMaps");

    if (!isAMapsMode && !isWMapsMode) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Unsupported Maps_Mode: " + Maps_Mode); }
    if (isAMapsMode && !Loaded_particle_AMap) { bt::ExitWithError(__func__, __FILE__, __LINE__, "AMaps destination container is null"); }
    if (isWMapsMode && !Loaded_particle_WMap) { bt::ExitWithError(__func__, __FILE__, __LINE__, "WMaps destination container is null"); }

    std::ifstream infile(filename);

    if (infile.is_open()) {
        const char* expectedPrefix = isAMapsMode ? "Line" : "Weight";
        std::string tp;
        int lineNumber = 0;

        while (getline(infile, tp)) {
            lineNumber++;
            stringstream ss(tp);
            std::string parameter, parameter2;
            ss >> parameter;

            if (bt::FindSubstring(parameter, expectedPrefix)) {
                ss >> parameter2;
                stringstream ss2(parameter2);

                std::string LineEntry;

                if (isAMapsMode) {
                    std::vector<int> col;

                    if (PrintOut) {
                        std::cout << "\n\nfilename = " << filename << "\n";
                        std::cout << "parameter = " << parameter << "\n";
                        std::cout << "parameter2 = " << parameter2 << "\n";
                        std::cout << "LineEntry = " << LineEntry << "\n\n";
                    }

                    while (getline(ss2, LineEntry, ':')) {
                        if (LineEntry.empty()) {
                            bt::ExitWithError(__func__, __FILE__, __LINE__, "Empty entry at line " + std::to_string(lineNumber) + " in file:\n" + filename + ":\n   -> " + tp);
                        }

                        try {
                            col.push_back(stoi(LineEntry));
                        } catch (const std::invalid_argument& e) { cerr << "Invalid integer found in file " << filename << ": " << LineEntry << "\n"; } catch (const std::out_of_range& e) {
                            cerr << "Integer out of range in file " << filename << ": " << LineEntry << "\n";
                        }
                    }

                    Loaded_particle_AMap->push_back(col);
                } else {
                    std::vector<double> col;

                    if (PrintOut) {
                        std::cout << "\n\nfilename = " << filename << "\n";
                        std::cout << "parameter = " << parameter << "\n";
                        std::cout << "parameter2 = " << parameter2 << "\n";
                        std::cout << "LineEntry = " << LineEntry << "\n\n";
                    }

                    while (getline(ss2, LineEntry, ':')) {
                        if (LineEntry.empty()) {
                            bt::ExitWithError(__func__, __FILE__, __LINE__, "Empty entry at line " + std::to_string(lineNumber) + " in file:\n" + filename + ":\n   -> " + tp);
                        }

                        try {
                            col.push_back(stoi(LineEntry));
                        } catch (const std::invalid_argument& e) { cerr << "Invalid double found in file " << filename << ": " << LineEntry << "\n"; } catch (const std::out_of_range& e) {
                            cerr << "Double out of range in file " << filename << ": " << LineEntry << "\n";
                        }
                    }

                    Loaded_particle_WMap->push_back(col);
                }
            }
        }
    } else {
        bt::PrintWarning(__func__, __FILE__, __LINE__, std::string("file:\n") + filename + "\nwas not found!");
    }
}
#pragma endregion

// MatchToAMap function -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* MatchToAMap function (original with neutron FC 'bug') */
bool AMaps::MatchToAMap(const std::string& Particle, double Momentum, double Theta, double Phi, bool NucleonOverlappingFC, bool UseExtendedElectronMaps) {
    int e_InitialSlice = 0, e_FinalSlice = Loaded_ElectronMomSliceLimits.size();
    int p_InitialSlice = 0, p_FinalSlice = Loaded_NucleonMomSliceLimits.size(), n_InitialSlice = 0, n_FinalSlice = Loaded_NucleonMomSliceLimits.size();

    int e_TestSlice = 1, p_TestSlice = 1, n_TestSlice = 1;

    if ((e_single_slice_test || nuc_single_slice_test) && Slices2Test.size() < 3) {
        bt::PrintWarning(__func__, __FILE__, __LINE__, "single-slice test mode requires TestSlices to contain 3 entries {e,p,n}; disabling single-slice tests for this run.");

        e_single_slice_test = false;
        nuc_single_slice_test = false;
    }

    if (Slices2Test.size() >= 3) {
        e_TestSlice = Slices2Test.at(0);
        p_TestSlice = Slices2Test.at(1);
        n_TestSlice = Slices2Test.at(2);
    }

    if (e_single_slice_test) { e_InitialSlice = e_TestSlice - 1, e_FinalSlice = e_TestSlice; }

    if (nuc_single_slice_test) { p_InitialSlice = p_TestSlice - 1, p_FinalSlice = p_TestSlice, n_InitialSlice = n_TestSlice - 1, n_FinalSlice = n_TestSlice; }

    if (isElectron(Particle)) {
        if (debugging::AMapsDebuggerMode) { std::cout << "\n\nLoaded_ElectronMomSliceLimits.size() = " << Loaded_ElectronMomSliceLimits.size() << "\n\n"; }

        for (int Slice = e_InitialSlice; Slice < e_FinalSlice; Slice++) {
            if (Momentum >= Loaded_ElectronMomSliceLimits.at(Slice).at(0) && Momentum <= Loaded_ElectronMomSliceLimits.at(Slice).at(1)) {
                for (int i = 0; i < HistElectronSliceNumOfYBins; i++) {
                    double dThetaTemp = (hBinUpperYLim - hBinLowerYLim) / HistElectronSliceNumOfYBins;
                    double ThetaLowerLimTemp = hBinLowerYLim + i * dThetaTemp;
                    double ThetaUpperLimTemp = ThetaLowerLimTemp + dThetaTemp;

                    if ((Theta >= ThetaLowerLimTemp) && (Theta < ThetaUpperLimTemp)) {
                        for (int j = 0; j < HistElectronSliceNumOfXBins; j++) {
                            double dPhiTemp = (hBinUpperXLim - hBinLowerXLim) / HistElectronSliceNumOfXBins;
                            double PhiLowerLimTemp = hBinLowerXLim + j * dPhiTemp;
                            double PhiUpperLimTemp = PhiLowerLimTemp + dPhiTemp;

                            if ((Phi >= PhiLowerLimTemp) && (Phi < PhiUpperLimTemp)) {
                                if (UseExtendedElectronMaps) {
                                    if (Loaded_e_AMap_Slices_extended.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_e_AMap_Slices_extended has size 0!"); }
                                    if (Loaded_e_AMap_Slices_extended.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_e_AMap_Slices_extended.at(" + std::to_string(Slice) + ") has size 0!");
                                    }

                                    if (debugging::AMapsDebuggerMode) {
                                        std::cout << "\n\nLoaded_e_AMap_Slices_extended.size() = " << Loaded_e_AMap_Slices_extended.size() << "\n";
                                        std::cout << "Loaded_e_AMap_Slices_extended.at(" << Slice << ").size() = " << Loaded_e_AMap_Slices_extended.at(Slice).size() << "\n";
                                        std::cout << "Loaded_e_AMap_Slices_extended.at(" << Slice << ").at(" << i << ").size() = " << Loaded_e_AMap_Slices_extended.at(Slice).at(i).size()
                                                  << "\n";
                                        std::cout << "Loaded_ElectronMomSliceLimits.at(" << Slice << ").at(0) = " << Loaded_ElectronMomSliceLimits.at(Slice).at(0) << "\n";
                                        std::cout << "Loaded_ElectronMomSliceLimits.at(" << Slice << ").at(1) = " << Loaded_ElectronMomSliceLimits.at(Slice).at(1) << "\n\n";
                                    }

                                    return (Loaded_e_AMap_Slices_extended.at(Slice).at(i).at(j) == 1);
                                } else {
                                    if (Loaded_e_AMap_Slices.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_e_AMap_Slices has size 0!"); }
                                    if (Loaded_e_AMap_Slices.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_e_AMap_Slices.at(" + std::to_string(Slice) + ") has size 0!");
                                    }

                                    if (debugging::AMapsDebuggerMode) {
                                        std::cout << "\n\nLoaded_e_AMap_Slices.size() = " << Loaded_e_AMap_Slices.size() << "\n";
                                        std::cout << "Loaded_e_AMap_Slices.at(" << Slice << ").size() = " << Loaded_e_AMap_Slices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_e_AMap_Slices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_e_AMap_Slices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_ElectronMomSliceLimits.at(" << Slice << ").at(0) = " << Loaded_ElectronMomSliceLimits.at(Slice).at(0) << "\n";
                                        std::cout << "Loaded_ElectronMomSliceLimits.at(" << Slice << ").at(1) = " << Loaded_ElectronMomSliceLimits.at(Slice).at(1) << "\n\n";
                                    }

                                    return (Loaded_e_AMap_Slices.at(Slice).at(i).at(j) == 1);
                                }
                            }  // end of find right phi if
                        }
                    }  // end of find right theta if
                }
            }  // end of if the right momentum
        }
    } else if (isProton(Particle)) {
        // TODO: figure out if neutron acceptance maps should be in momentum slices or not, due to bin migration
        // TODO: decide which neutron maps to use here - const or not

        if (debugging::AMapsDebuggerMode) { std::cout << "\n\nLoaded_NucleonMomSliceLimits.size() = " << Loaded_NucleonMomSliceLimits.size() << "\n\n"; }

        for (int Slice = p_InitialSlice; Slice < p_FinalSlice; Slice++) {
            if (Momentum >= Loaded_NucleonMomSliceLimits.at(Slice).at(0) && Momentum <= Loaded_NucleonMomSliceLimits.at(Slice).at(1)) {
                for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                    double dThetaTemp = (hBinUpperYLim - hBinLowerYLim) / (HistNucSliceNumOfYBins);
                    double ThetaLowerLimTemp = hBinLowerYLim + i * dThetaTemp;
                    double ThetaUpperLimTemp = ThetaLowerLimTemp + dThetaTemp;

                    if ((Theta >= ThetaLowerLimTemp) && (Theta < ThetaUpperLimTemp)) {
                        for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                            double dPhiTemp = (hBinUpperXLim - hBinLowerXLim) / (HistNucSliceNumOfXBins);
                            double PhiLowerLimTemp = hBinLowerXLim + j * dPhiTemp;
                            double PhiUpperLimTemp = PhiLowerLimTemp + dPhiTemp;

                            if ((Phi >= PhiLowerLimTemp) && (Phi < PhiUpperLimTemp)) {
                                if (NucleonOverlappingFC) {
                                    // Safety checks:
                                    if (Loaded_p_AMap_Slices.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_p_AMap_Slices has size 0!"); }
                                    if (Loaded_n_AMap_ConstSlices.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_n_AMap_ConstSlices has size 0!"); }

                                    if (Loaded_p_AMap_Slices.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_p_AMap_Slices.at(" + std::to_string(Slice) + ") has size 0!");
                                    }
                                    if (Loaded_n_AMap_ConstSlices.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_n_AMap_ConstSlices.at(" + std::to_string(Slice) + ") has size 0!");
                                    }

                                    if (debugging::AMapsDebuggerMode) {
                                        std::cout << "\n\nLoaded_n_AMap_ConstSlices.size() = " << Loaded_n_AMap_ConstSlices.size() << "\n";
                                        std::cout << "Loaded_n_AMap_ConstSlices.at(" << Slice << ").size() = " << Loaded_n_AMap_ConstSlices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_n_AMap_ConstSlices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_n_AMap_ConstSlices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.size() = " << Loaded_p_AMap_Slices.size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.at(" << Slice << ").size() = " << Loaded_p_AMap_Slices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_p_AMap_Slices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(0) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(0) << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(1) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(1) << "\n\n";
                                    }

                                    return ((Loaded_p_AMap_Slices.at(Slice).at(i).at(j) == 1) && (Loaded_n_AMap_ConstSlices.at(Slice).at(i).at(j) == 1));
                                } else {
                                    if (debugging::AMapsDebuggerMode) {
                                        std::cout << "\nLoaded_p_AMap_Slices.at(" << Slice << ").size() = " << Loaded_p_AMap_Slices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_p_AMap_Slices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(0) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(0) << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(1) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(1) << "\n\n";
                                    }

                                    return (Loaded_p_AMap_Slices.at(Slice).at(i).at(j) == 1);
                                }
                            }  // end of find right phi if
                        }
                    }  // end of find right theta if
                }
            }  // end of if the right momentum
        }
    } else if (isNeutron(Particle)) {
        // TODO: figure out if neutron acceptance maps should be in momentum slices or not, due to bin migration
        // TODO: decide which neutron maps to use here - const or not

        if (debugging::AMapsDebuggerMode) { std::cout << "\n\nLoaded_NucleonMomSliceLimits.size() = " << Loaded_NucleonMomSliceLimits.size() << "\n\n"; }

        for (int Slice = n_InitialSlice; Slice < n_FinalSlice; Slice++) {
            if (Momentum >= Loaded_NucleonMomSliceLimits.at(Slice).at(0) && Momentum <= Loaded_NucleonMomSliceLimits.at(Slice).at(1)) {
                for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                    double dThetaTemp = (hBinUpperYLim - hBinLowerYLim) / (HistNucSliceNumOfYBins);
                    double ThetaLowerLimTemp = hBinLowerYLim + i * dThetaTemp;
                    double ThetaUpperLimTemp = ThetaLowerLimTemp + dThetaTemp;

                    if ((Theta >= ThetaLowerLimTemp) && (Theta < ThetaUpperLimTemp)) {
                        for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                            double dPhiTemp = (hBinUpperXLim - hBinLowerXLim) / (HistNucSliceNumOfXBins);
                            double PhiLowerLimTemp = hBinLowerXLim + j * dPhiTemp;
                            double PhiUpperLimTemp = PhiLowerLimTemp + dPhiTemp;

                            if ((Phi >= PhiLowerLimTemp) && (Phi < PhiUpperLimTemp)) {
                                if (NucleonOverlappingFC) {
                                    // Safety checks:
                                    if (Loaded_p_AMap_Slices.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_p_AMap_Slices has size 0!"); }
                                    if (Loaded_n_AMap_ConstSlices.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_n_AMap_ConstSlices has size 0!"); }

                                    if (Loaded_p_AMap_Slices.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_p_AMap_Slices.at(" + std::to_string(Slice) + ") has size 0!");
                                    }
                                    if (Loaded_n_AMap_ConstSlices.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_n_AMap_ConstSlices.at(" + std::to_string(Slice) + ") has size 0!");
                                    }

                                    if (debugging::AMapsDebuggerMode) {
                                        std::cout << "\n\nLoaded_n_AMap_ConstSlices.size() = " << Loaded_n_AMap_ConstSlices.size() << "\n";
                                        std::cout << "Loaded_n_AMap_ConstSlices.at(" << Slice << ").size() = " << Loaded_n_AMap_ConstSlices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_n_AMap_ConstSlices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_n_AMap_ConstSlices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.size() = " << Loaded_p_AMap_Slices.size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.at(" << Slice << ").size() = " << Loaded_p_AMap_Slices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_p_AMap_Slices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_p_AMap_Slices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(0) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(0) << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(1) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(1) << "\n\n";
                                    }

                                    return ((Loaded_p_AMap_Slices.at(Slice).at(i).at(j) == 1) && (Loaded_n_AMap_ConstSlices.at(Slice).at(i).at(j) == 1));
                                } else {
                                    // Safety check:
                                    if (Loaded_n_AMap_ConstSlices.size() == 0) { bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_n_AMap_ConstSlices has size 0!"); }

                                    if (Loaded_n_AMap_ConstSlices.at(Slice).size() == 0) {
                                        bt::ExitWithError(__func__, __FILE__, __LINE__, "Loaded_n_AMap_ConstSlices.at(" + std::to_string(Slice) + ") has size 0!");
                                    }

                                    if (debugging::AMapsDebuggerMode) {
                                        std::cout << "\n\nLoaded_n_AMap_ConstSlices.size() = " << Loaded_n_AMap_ConstSlices.size() << "\n";
                                        std::cout << "Loaded_n_AMap_ConstSlices.at(" << Slice << ").size() = " << Loaded_n_AMap_ConstSlices.at(Slice).size() << "\n";
                                        std::cout << "Loaded_n_AMap_ConstSlices.at(" << Slice << ").at(" << i << ").size() = " << Loaded_n_AMap_ConstSlices.at(Slice).at(i).size() << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(0) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(0) << "\n";
                                        std::cout << "Loaded_NucleonMomSliceLimits.at(" << Slice << ").at(1) = " << Loaded_NucleonMomSliceLimits.at(Slice).at(1) << "\n\n";
                                    }

                                    return (Loaded_n_AMap_ConstSlices.at(Slice).at(i).at(j) == 1);
                                }
                            }  // end of find right phi if
                        }
                    }  // end of find right theta if
                }
            }  // end of if the right momentum
        }
    }  // end of if Particle

    return false;
}
#pragma endregion

// GetWeight function ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GetWeight function */
double AMaps::GetWeight(bool apply_kinematical_weights, const std::string& Particle, double Momentum, double Theta, double Phi) {
    bool PrintOut = false;

    if (apply_kinematical_weights) {
        if (isElectron(Particle)) {
            return 1;
        } else if (isProton(Particle)) {
            for (int Slice = 0; Slice < Loaded_NucleonMomSliceLimits.size(); Slice++) {
                if (Momentum >= Loaded_NucleonMomSliceLimits.at(Slice).at(0) && Momentum <= Loaded_NucleonMomSliceLimits.at(Slice).at(1)) {
                    for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                        double dThetaTemp = (hBinUpperYLim - hBinLowerYLim) / (HistNucSliceNumOfYBins);
                        double ThetaLowerLimTemp = hBinLowerYLim + i * dThetaTemp;
                        double ThetaUpperLimTemp = ThetaLowerLimTemp + dThetaTemp;

                        if ((Theta >= ThetaLowerLimTemp) && (Theta < ThetaUpperLimTemp)) {
                            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                                double dPhiTemp = (hBinUpperXLim - hBinLowerXLim) / (HistNucSliceNumOfXBins);
                                double PhiLowerLimTemp = hBinLowerXLim + j * dPhiTemp;
                                double PhiUpperLimTemp = PhiLowerLimTemp + dPhiTemp;

                                if ((Phi >= PhiLowerLimTemp) && (Phi < PhiUpperLimTemp)) {
                                    double Weight;

                                    if (Loaded_p_WMap_Slices.at(Slice).at(i).at(j) != 0) {
                                        Weight = (1 / (Loaded_p_WMap_Slices.at(Slice).at(i).at(j)));

                                        if (PrintOut) {
                                            std::cout << "MomentumLowerLimTemp = " << Loaded_NucleonMomSliceLimits.at(Slice).at(0) << "\n";
                                            std::cout << "Momentum = " << Momentum << "\n";
                                            std::cout << "MomentumUpperLimTemp = " << Loaded_NucleonMomSliceLimits.at(Slice).at(1) << "\n\n";

                                            std::cout << "ThetaLowerLimTemp = " << ThetaLowerLimTemp << "\n";
                                            std::cout << "Theta = " << Theta << "\n";
                                            std::cout << "ThetaUpperLimTemp = " << ThetaUpperLimTemp << "\n\n";

                                            std::cout << "PhiLowerLimTemp = " << PhiLowerLimTemp << "\n";
                                            std::cout << "Phi = " << Phi << "\n";
                                            std::cout << "PhiUpperLimTemp = " << PhiUpperLimTemp << "\n\n";

                                            std::cout << "Weight = " << Weight << "\n\n";

                                            std::cout << "1/Weight = " << (1 / Weight) << "\n\n\n\n";
                                        }
                                    } else {
                                        Weight = 0;
                                    }

                                    return Weight;
                                }  // end of find right phi if
                            }
                        }  // end of find right theta if
                    }
                }  // end of if the right momentum
            }
        } else if (isNeutron(Particle)) {
            // TODO: figure out if neutron weight maps should be in momentum slices or not, due to bin migration
            // TODO: decide which neutron maps to use here - const or not

            for (int Slice = 0; Slice < Loaded_NucleonMomSliceLimits.size(); Slice++) {
                if (Momentum >= Loaded_NucleonMomSliceLimits.at(Slice).at(0) && Momentum <= Loaded_NucleonMomSliceLimits.at(Slice).at(1)) {
                    for (int i = 0; i < HistNucSliceNumOfYBins; i++) {
                        double dThetaTemp = (hBinUpperYLim - hBinLowerYLim) / (HistNucSliceNumOfYBins);
                        double ThetaLowerLimTemp = hBinLowerYLim + i * dThetaTemp;
                        double ThetaUpperLimTemp = ThetaLowerLimTemp + dThetaTemp;

                        if ((Theta >= ThetaLowerLimTemp) && (Theta < ThetaUpperLimTemp)) {
                            for (int j = 0; j < HistNucSliceNumOfXBins; j++) {
                                double dPhiTemp = (hBinUpperXLim - hBinLowerXLim) / (HistNucSliceNumOfXBins);
                                double PhiLowerLimTemp = hBinLowerXLim + j * dPhiTemp;
                                double PhiUpperLimTemp = PhiLowerLimTemp + dPhiTemp;

                                if ((Phi >= PhiLowerLimTemp) && (Phi < PhiUpperLimTemp)) {
                                    double Weight;

                                    if (Loaded_n_WMap_Slices.at(Slice).at(i).at(j) != 0) {
                                        Weight = (1 / (Loaded_n_WMap_Slices.at(Slice).at(i).at(j)));

                                        if (PrintOut) {
                                            std::cout << "MomentumLowerLimTemp = " << Loaded_NucleonMomSliceLimits.at(Slice).at(0) << "\n";
                                            std::cout << "Momentum = " << Momentum << "\n";
                                            std::cout << "MomentumUpperLimTemp = " << Loaded_NucleonMomSliceLimits.at(Slice).at(1) << "\n\n";

                                            std::cout << "ThetaLowerLimTemp = " << ThetaLowerLimTemp << "\n";
                                            std::cout << "Theta = " << Theta << "\n";
                                            std::cout << "ThetaUpperLimTemp = " << ThetaUpperLimTemp << "\n\n";

                                            std::cout << "PhiLowerLimTemp = " << PhiLowerLimTemp << "\n";
                                            std::cout << "Phi = " << Phi << "\n";
                                            std::cout << "PhiUpperLimTemp = " << PhiUpperLimTemp << "\n\n";

                                            std::cout << "Weight = " << Weight << "\n\n";

                                            std::cout << "1/Weight = " << (1 / Weight) << "\n\n\n\n";
                                        }
                                    } else {
                                        Weight = 0;
                                    }

                                    return Weight;
                                }  // end of find right phi if
                            }
                        }  // end of find right theta if
                    }
                }  // end of if the right momentum
            }
        }  // end of if Particle

        return 0;
    } else {
        return 1;
    }
}
#pragma endregion

// IsInFDQuery function -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* IsInFDQuery function */
bool AMaps::IsInFDQuery(bool Generate_AMaps, const DSCuts& ThetaFD, const std::string& Particle, double Momentum, double Theta, double Phi, bool NucleonOverlappingFC,
                        bool UseExtendedElectronMaps) {
    std::string DebuggingBaseString = "[AMaps::IsInFDQuery]: " + Particle + "\nMomentum: " + bt::ToStringWithPrecision(Momentum, 2) +
                                      " [GeV/c]\nTheta: " + bt::ToStringWithPrecision(Theta, 2) + " [rad]\nPhi: " + bt::ToStringWithPrecision(Phi, 2) + " deg\nMode = " + Maps_Mode;
    debugging::CodeDebugger.PrintStepTester(__FILE__, __LINE__, DebuggerMode, OnlyPrintNamedTesterSteps, (DebuggingBaseString + " - Start"));

    bool inFDQuery, part_inSomeSector;

    if (!Generate_AMaps) {
        part_inSomeSector = MatchToAMap(Particle, Momentum, Theta, Phi, NucleonOverlappingFC, UseExtendedElectronMaps);
    } else {
        part_inSomeSector = true;
    }

    inFDQuery = (part_inSomeSector && (Theta >= ThetaFD.GetLowerCutConst()) && (Theta <= ThetaFD.GetUpperCutConst()));

    debugging::CodeDebugger.PrintStepTester(__FILE__, __LINE__, DebuggerMode, OnlyPrintNamedTesterSteps, (DebuggingBaseString + " - End"));

    return inFDQuery;
}
#pragma endregion

#endif  // AMAPS_H
