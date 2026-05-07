//
// Created by Alon Sportes on 22/06/2023.
//

// #ifndef AMAPS_H
// #define AMAPS_H
#ifdef AMAPS_H

    #include <TApplication.h>
    #include <TCanvas.h>
    #include <TChain.h>
    #include <TDatabasePDG.h>
    #include <TF1.h>
    #include <TFile.h>
    #include <TH1.h>
    #include <TH2.h>
    #include <TLatex.h>
    #include <TLorentzVector.h>
    #include <TROOT.h>
    #include <TStyle.h>
    #include <TTree.h>
    #include <math.h>

    #include <chrono>
    #include <cstdlib>
    #include <fstream>
    #include <initializer_list>
    #include <iomanip>
    #include <iostream>
    #include <map>
    #include <sstream>
    #include <string>
    #include <typeinfo>
    #include <vector>

    // Include libraries:
    #include "../../namespaces/general_utilities/utilities.h"
    #include "../../namespaces/setup/debugging.h"

    // Include classes:
    #include "../DSCuts/DSCuts.h"
    #include "../hPlots/hPlot2D.cpp"
    #include "CustomSliceLimits.h"

using namespace debugging;
using namespace utilities;

namespace bt = basic_tools;
namespace hf = histogram_functions;

/**
 * @class AMaps
 * @brief Generates, saves, loads, and applies acceptance maps (AMaps) and weight maps (WMaps).
 * @details AMaps has two runtime modes:
 * - "AMaps": binary acceptance maps (0/1, thresholded from Reco/TL).
 * - "WMaps": continuous weight maps used for acceptance correction.
 *
 * For each momentum slice, the class stores truth/reco/ratio histograms and map matrices.
 * It supports map generation from events, map I/O (`.par` files), ROOT/PDF outputs, and
 * acceptance/weight queries used during analysis.
 *
 * `LoadWMapsFromRatioROOTFiles(...)` is used to reconstruct WMap `.par` files directly
 * from saved ratio ROOT files (`AMapsRatio` and `ConstAMapsRatio`).
 */
class AMaps {
   public:
    /**
     * @enum ParticleType
     * @brief Defines the type of particle to which we generate/load the maps
     */
    enum ParticleType {
        ELECTRON_TYPE,  // Electrons
        PROTON_TYPE,    // Protons
        NEUTRON_TYPE,   // Neutrons
        UNDEFINED_TYPE  // Other particles
    };

    // default constructor ----------------------------------------------------------------------------------------------------------------------------------------------

    AMaps() : particleType(UNDEFINED_TYPE) {}

    // constructor ------------------------------------------------------------------------------------------------------------------------------------------------------

    // AMaps generation constructor:
    AMaps(const std::string& SampleName, const std::string& P_e_bin_profile, const std::string& P_nuc_bin_profile, double beamE, const std::string& AMapsMode = "",
          const std::string& SavePath = "./", int nOfNucMomBins = 4, int nOfElecMomBins = 4, int hnsNumOfXBins = 75, int hnsNumOfYBins = 75, int hesNumOfXBins = 100, int hesNumOfYBins = 100,
          const std::string& ElectronLabel = "e", const std::string& ProtonLabel = "pFD", const std::string& NeutronLabel = "nFD", ParticleType type = UNDEFINED_TYPE);

    // AMaps loading constructor:
    AMaps(const std::string& AcceptanceMapsDirectory, const double& beamE, const std::string& AMapsMode, const bool& Electron_single_slice_test, const bool& Nucleon_single_slice_test,
          const std::vector<int>& TestSlices, ParticleType type = UNDEFINED_TYPE);

    // SetBins functions ------------------------------------------------------------------------------------------------------------------------------------------------

    void SetBins(const std::string& P_nuc_bin_profile, double beamE);

    void SetElectronBins(const std::string& P_e_bin_profile, double beamE);

    void SetBins(double beamE, double NumberNucOfMomSlices);  // old

    // isElectron function ----------------------------------------------------------------------------------------------------------------------------------------------

    bool isElectron(const std::string& SampleType);

    // isProton function ------------------------------------------------------------------------------------------------------------------------------------------------

    bool isProton(const std::string& SampleType);

    // isNeutron function -----------------------------------------------------------------------------------------------------------------------------------------------

    bool isNeutron(const std::string& SampleType);

    // isTL function ----------------------------------------------------------------------------------------------------------------------------------------------------

    bool isTL(const std::string& SampleType);

    // isReco function --------------------------------------------------------------------------------------------------------------------------------------------------

    bool isReco(const std::string& SampleType);

    // hFillMaps function -----------------------------------------------------------------------------------------------------------------------------------------------

    void hFillMaps(const std::string& SampleType, const std::string& particle, double Momentum, double Theta, double Phi, double Weight);

    // CalcAcceptanceEfficiency function --------------------------------------------------------------------------------------------------------------------------------

    void CalcAcceptanceEfficiency();

    // GenerateFilteredRecoMaps function --------------------------------------------------------------------------------------------------------------------------------

    void GenerateFilteredRecoMaps(double cP_minR, double nP_minR);

    // GenerateMapMatrices function -------------------------------------------------------------------------------------------------------------------------------------

    void GenerateMapMatrices(double cP_minR, double nP_minR);

    // GenerateExtendedElectronMapMatrices function ---------------------------------------------------------------------------------------------------------------------

    void GenerateExtendedElectronMapMatrices(const std::vector<std::vector<std::vector<int>>>& ref_e_AMap_Slices);

    // GenerateFilteredRecoCPartMaps function ---------------------------------------------------------------------------------------------------------------------------

    void GenerateFilteredRecoCPartMaps(double cP_minR);

    // GenerateCPartAMaps function --------------------------------------------------------------------------------------------------------------------------------------

    void GenerateCPartAMaps(double cP_minR);

    // GenerateNPartAMaps function --------------------------------------------------------------------------------------------------------------------------------------

    void GenerateNPartAMaps(double nP_minR);

    // GenerateNucleonAMap function -------------------------------------------------------------------------------------------------------------------------------------

    void GenerateNucleonAMap();

    // SaveHitMaps function ---------------------------------------------------------------------------------------------------------------------------------------------

    void SaveHitMaps(const std::string& SampleName, const std::string& AcceptanceMapsDirectory);

    // SaveMaps function ------------------------------------------------------------------------------------------------------------------------------------------------
    // Writes mode-dependent map slices to `.par` files (AMaps -> `Line`, WMaps -> `Weight`).

    void SaveMaps(const std::string& SampleName, const std::string& AcceptanceMapsDirectory);

    // DrawAndSaveMapsBySliceToPDFs function ----------------------------------------------------------------------------------------------------------------------------

    void DrawAndSaveMapsBySliceToPDFs(std::vector<TObject*> HistoList, const std::string& PDFfileName);

    // DrawAndSaveMaps function -----------------------------------------------------------------------------------------------------------------------------------------

    void DrawAndSaveMaps(const std::string& SampleName, TCanvas* h1DCanvas, const std::string& AcceptanceMapsDirectory);

    // HistCounter function ---------------------------------------------------------------------------------------------------------------------------------------------

    int HistCounter(const char* fname);

    // SetHistBinsFromHistTitle function --------------------------------------------------------------------------------------------------------------------------------

    void SetHistBinsFromHistTitle(TH2D* Histogram2D);

    // SetSlicesFromHistTitle function ----------------------------------------------------------------------------------------------------------------------------------

    void SetSlicesFromHistTitle(TH2D* Histogram2D, const std::string& Particle);

    void SetSlicesFromHistTitle(TH2D* Histogram2D, std::vector<std::vector<double>> MomBinsLimits);

    // ReadHitMaps function ---------------------------------------------------------------------------------------------------------------------------------------------

    void ReadHitMaps(const std::string& AcceptanceMapsDirectory, const std::string& SampleName);

    // LoadWMapsFromRatioROOTFiles function -----------------------------------------------------------------------------------------------------------------------------
    // Rebuilds WMap matrices from ratio ROOT files and writes WMap `.par` slices.
    // InputAcceptanceMapsDirectory is read-only source path.
    // OutputAcceptanceMapsDirectory is write path (if empty, input path is reused).
    // Thresholds used for WMap export:
    // - e/p: keep ratio >= 0.7
    // - n:   keep ratio >= 0.2

    void LoadWMapsFromRatioROOTFiles(const std::string& SampleName, const std::string& InputAcceptanceMapsDirectory, const std::string& OutputAcceptanceMapsDirectory = "");

    // ReadAMapLimits function ------------------------------------------------------------------------------------------------------------------------------------------

    void ReadAMapLimits(const char* filename, std::vector<std::vector<double>>& Loaded_particle_limits);

    // ReadMapSlices function -------------------------------------------------------------------------------------------------------------------------------------------
    // Loads all momentum slices for one particle from `.par` files into AMap or WMap containers.

    void ReadMapSlices(const std::string& SampleName, const std::string& AcceptanceMapsDirectory, const std::string& Particle, const std::vector<std::vector<double>>& Loaded_particle_limits,
                       std::vector<std::vector<std::vector<int>>>* Loaded_Particle_AMap_Slices = nullptr,
                       std::vector<std::vector<std::vector<double>>>* Loaded_Particle_WMap_Slices = nullptr, const bool Special_maps = false);

    // ReadMap function -------------------------------------------------------------------------------------------------------------------------------------------------
    // Loads a single `.par` map into AMap or WMap container according to Maps_Mode.

    void ReadMap(const char* filename, std::vector<std::vector<int>>* Loaded_particle_AMap = nullptr, std::vector<std::vector<double>>* Loaded_particle_WMap = nullptr);

    // MatchToAMap function ---------------------------------------------------------------------------------------------------------------------------------------------

    bool MatchToAMap(const std::string& Particle, double Momentum, double Theta, double Phi, bool NucleonOverlappingFC = true, bool UseExtendedElectronMaps = false);

    // GetWeight function -----------------------------------------------------------------------------------------------------------------------------------------------

    double GetWeight(bool apply_kinematical_weights, const std::string& Particle, double Momentum, double Theta, double Phi);

    // IsInFDQuery function ---------------------------------------------------------------------------------------------------------------------------------------------

    bool IsInFDQuery(bool Generate_AMaps, const DSCuts& ThetaFD, const std::string& Particle, double Momentum, double Theta, double Phi, bool NucleonOverlappingFC = true,
                     bool UseExtendedElectronMaps = false);

    // Temporary functions for histogram title conversion for DrawAndSaveMapsBySliceToPDFs -------------------------------------------------------------------------------

    // TODO: generalize histogram titles so you can remove these temporary functions later

    #include <regex>
    #include <string>

    // Replace all semicolons so ROOT won't parse them as axis-title delimiters
    static std::string sanitize_for_root_title(std::string s) {
        for (auto& ch : s)
            if (ch == ';') {
                ch = ',';  // or use '·'
            }

        return s;
    }

    static std::string make_header(const std::string& level, const std::string& particle, const std::string& grid, const std::string& mode) {
        // NOTE: use a comma instead of a semicolon inside the parentheses
        std::string header = "#theta^{" + level + "}_{" + particle + "} vs. #phi^{" + level + "}_{" + particle + "} of " + level + " P_{" + particle + "} (" + grid + ", " + mode + ")";

        return sanitize_for_root_title(header);
    }

    static std::string convert_title_to_splitline(const std::string& t) {
        if (t.find("#splitline") != std::string::npos) { return t; }

        static const std::regex rx(R"(^\s*(Reco|TL|Truth)\s+P_\{([epn])\}\s+bin\s+for\s+([^\(]*?\[GeV/c\])\s+\((\d+x\d+)\)\s+\((AMaps|WMaps)\)\s*$)");

        std::smatch m;

        if (!std::regex_match(t, m, rx) || m.size() != 6) return t;

        const std::string level_token = m[1];  // "Reco"/"TL"/"Truth"
        const std::string particle = m[2];     // "e"/"p"/"n"
        const std::string slice = m[3];        // "... [GeV/c]"
        const std::string grid = m[4];         // "100x100"
        const std::string mode = m[5];         // "AMaps"/"WMaps"

        const std::string level_lower = (level_token == "Reco") ? "reco" : (level_token == "TL") ? "truth" : "truth";

        const std::string header = make_header(level_lower, particle, grid, mode);
        const std::string slice_sanitized = sanitize_for_root_title(slice);

        return "#splitline{ " + header + " }{ Slice limits: " + slice_sanitized + " }";
    }

    // Apply to a list of histograms while preserving axis labels
    void RewriteTitlesToSplitline(const std::vector<TObject*>& objs) {
        for (TObject* obj : objs) {
            if (!obj) { continue; }

            if (TH1* h = dynamic_cast<TH1*>(obj)) {
                // Save current axis labels
                std::string xlab = h->GetXaxis() ? h->GetXaxis()->GetTitle() : "";
                std::string ylab = h->GetYaxis() ? h->GetYaxis()->GetTitle() : "";
                std::string zlab;

                if (auto* h2 = dynamic_cast<TH2*>(h)) {
                    if (h2->GetZaxis()) { zlab = h2->GetZaxis()->GetTitle(); }
                }

                // Convert title (and sanitize)
                const char* ct = h->GetTitle();
                const std::string curr = ct ? std::string(ct) : std::string();
                const std::string converted = convert_title_to_splitline(curr);

                if (converted != curr) {
                    h->SetTitle(converted.c_str());  // Set only the title (no semicolons inside!)
                    // Restore axis labels explicitly
                    if (h->GetXaxis()) h->GetXaxis()->SetTitle(xlab.c_str());
                    if (h->GetYaxis()) h->GetYaxis()->SetTitle(ylab.c_str());
                    if (auto* h2 = dynamic_cast<TH2*>(h)) {
                        if (h2->GetZaxis()) { h2->GetZaxis()->SetTitle(zlab.c_str()); }
                    }
                }
            }
        }
    }

    // Other methods ----------------------------------------------------------------------------------------------------------------------------------------------------

    // Set methods
    void SetNPartMinRatio(double npmr) { Neutral_particle_min_Ratio = npmr; }

    void SetCPartMinRatio(double cpmr) { Charged_particle_min_Ratio = cpmr; }

    // Get methods
    double GetNPartMinRatio() { return Neutral_particle_min_Ratio; }

    double GetCPartMinRatio() { return Charged_particle_min_Ratio; }

    int GetElectronMomSliceLimitsSize() { return ElectronMomSliceLimits.size(); }

    int GetNucleonMomSliceLimitsSize() { return NucleonMomSliceLimits.size(); }

    int GetHistElectronSliceNumOfXBins() { return HistElectronSliceNumOfXBins; }

    int GetHistElectronSliceNumOfYBins() { return HistElectronSliceNumOfYBins; }

    int GetHistNucSliceNumOfXBins() { return HistNucSliceNumOfXBins; }

    int GetHistNucSliceNumOfYBins() { return HistNucSliceNumOfYBins; }

    std::vector<std::vector<double>> GetElectronMomSliceLimits() { return ElectronMomSliceLimits; }

    std::vector<std::vector<double>> GetNucleonMomSliceLimits() { return NucleonMomSliceLimits; }

    std::vector<std::vector<double>> GetLoadedElectronMomSliceLimits() { return Loaded_ElectronMomSliceLimits; }

    std::vector<std::vector<double>> GetLoadedNucleonMomSliceLimits() { return Loaded_NucleonMomSliceLimits; }

   private:
    static bool IsOneOf(const std::string& value, std::initializer_list<const char*> allowedValues);

    ParticleType particleType;

    /* Particle labels */
    // These labels are used to identify particles in the sample types (e.g., "e" for electron, "pFD" for proton in FD, "nFD" for neutron in FD, etc.)
    // They can be customized via the constructor parameters if the user wants to generate/load AMaps for the CD (relevant only for protons and neutrons).
    std::string eLabel = "e";
    std::string pLabel = "pFD";
    std::string nLabel = "nFD";

    /* Acceptance maps mode */
    // Possible modes: "AMaps" - full acceptance maps generation/loading; "WMaps" - weight used to apply acceptance correction
    std::string Maps_Mode = "AMaps";

    /* Acceptance maps from class and before cuts (to be compared with one generated with the file) */
    hPlot2D reco_theta_e_VS_phi_e_BC, reco_theta_p_VS_phi_p_BC, reco_theta_n_VS_phi_n_BC, reco_theta_nuc_VS_phi_nuc_BC;

    std::vector<std::vector<double>> InvertedElectronMomSliceLimits, ElectronMomSliceLimits;
    std::vector<std::vector<double>> InvertedNucleonMomSliceLimits, NucleonMomSliceLimits;

    double hBinLowerXLim = -180, hBinUpperXLim = 180;
    double hBinLowerYLim = 0, hBinUpperYLim = 50;

    int HistElectronSliceNumOfXBins;  // 100 by Default
    int HistElectronSliceNumOfYBins;  // 100 by Default
    int HistNucSliceNumOfXBins;       // 75 by Default
    int HistNucSliceNumOfYBins;       // 75 by Default
    int NumberNucOfMomSlices;
    int NumberElecOfMomSlices;

    double Nucleon_Momentum_Slice_Th = 0.4;
    double Electron_Momentum_Slice_Th = 0.8;  // Not th! but distance from beamE of last mom bin!

    /* Truth-level theta VS phi */
    std::vector<hPlot2D> truth_theta_e_VS_phi_e_BySlice, truth_theta_p_VS_phi_p_BySlice, truth_theta_n_VS_phi_n_BySlice, truth_theta_n_VS_phi_n_ByConstSlice;
    hPlot2D truth_theta_n_VS_phi_n;
    std::vector<TObject*> truth_e_BySlice, truth_p_BySlice, truth_n_BySlice, truth_n_ByConstSlice;

    /* Reco. Acceptance maps */
    std::vector<hPlot2D> reco_theta_e_VS_phi_e_BySlice, reco_theta_p_VS_phi_p_BySlice, reco_theta_n_VS_phi_n_BySlice, reco_theta_n_VS_phi_n_ByConstSlice;
    hPlot2D reco_theta_n_VS_phi_n;
    std::vector<TObject*> reco_e_BySlice, reco_p_BySlice, reco_n_BySlice, reco_n_ByConstSlice;

    /* Reco./TL ratio maps */
    std::vector<hPlot2D> acceptance_eff_e_BySlice, acceptance_eff_p_BySlice, acceptance_eff_n_BySlice, acceptance_eff_n_ByConstSlice;
    hPlot2D acceptance_eff_n;
    std::vector<TObject*> acc_eff_e_BySlice, acc_eff_p_BySlice, acc_eff_n_BySlice, acc_eff_n_ByConstSlice;

    /* Separated acceptance & weight maps (= reco. above ratio th.) */
    std::vector<hPlot2D> filtered_reco_theta_e_VS_phi_e_BySlice, filtered_reco_theta_p_VS_phi_p_BySlice, filtered_reco_theta_n_VS_phi_n_BySlice,
        filtered_reco_theta_n_VS_phi_n_ByConstSlice;  // separated AMaps for each slice
    std::vector<TObject*> filtered_reco_e_BySlice, filtered_reco_p_BySlice, filtered_reco_n_BySlice, filtered_reco_n_ByConstSlice;

    /* Acceptance maps (= reco. above ratio th.) */
    hPlot2D filtered_reco_theta_e_VS_phi_e;  // combined electron AMap
    hPlot2D filtered_reco_theta_p_VS_phi_p;  // combined proton AMap
    hPlot2D filtered_reco_theta_n_VS_phi_n;
    hPlot2D filtered_reco_theta_nuc_VS_phi_nuc;

    /* Acceptance maps */
    std::vector<std::vector<int>> e_AMap;
    std::vector<std::vector<std::vector<int>>> e_AMap_Slices, e_AMap_Slices_extended;
    std::vector<std::vector<int>> p_AMap;
    std::vector<std::vector<std::vector<int>>> p_AMap_Slices;
    std::vector<std::vector<int>> n_AMap;
    std::vector<std::vector<std::vector<int>>> n_AMap_Slices, n_AMap_ConstSlices;
    std::vector<std::vector<int>> nuc_AMap;
    std::vector<std::vector<std::vector<int>>> nuc_AMap_Slices;
    std::vector<std::vector<std::vector<int>>> nuc_WMap_Slices;  // TODO: figure out if really need these!

    /* Weight maps */
    std::vector<std::vector<std::vector<double>>> e_WMap_Slices;
    std::vector<std::vector<std::vector<double>>> p_WMap_Slices;
    std::vector<std::vector<double>> n_WMap;
    std::vector<std::vector<std::vector<double>>> n_WMap_Slices, n_WMap_ConstSlices;

    double Charged_particle_min_Ratio = 0.7;
    double Neutral_particle_min_Ratio = 0.2;

    TList* AcceptanceMapsBC = new TList();
    TList* TLAMaps = new TList();
    TList* ConstTLAMaps = new TList();
    TList* RecoAMaps = new TList();
    TList* ConstRecoAMaps = new TList();
    TList* AMapsRatio = new TList();
    TList* ConstAMapsRatio = new TList();
    TList* Charged_particle_Sep_AMaps = new TList();
    TList* Const_neutral_particle_Sep_AMaps = new TList();
    TList* AcceptanceMaps = new TList();

    int pixelx = 1980, pixely = 1530;

    std::string AcceptanceMapsBC_OutFile0;
    std::string TLAMaps_OutFile0;
    std::string RecoAMaps_OutFile0;
    std::string AMapsRatio_OutFile0;
    std::string Charged_particle_Sep_AMaps_OutFile0;
    std::string AcceptanceMaps_OutFile0;

    std::string AMapSavePath, AMapCopySavePath;

    std::string AMapsBC_prefix = "00_AMaps_BC_-_";
    std::string AMap_TL_prefix = "01_AMap_TL_-_";
    std::string AMap_Reco_prefix = "02_AMap_Reco_-_";
    std::string AMap_Ratio_prefix = "03_AMap_Ratio_-_";
    std::string cPart_Sep_AMaps_prefix = "04_cPart_Sep_AMaps_-_";
    std::string nPart_Sep_AMaps_prefix = "04_nPart_Sep_AMaps_-_";
    std::string AMaps_prefix = "05_AMaps_-_";

    std::vector<TObject*> AMapsBC_HistoList;
    std::vector<TObject*> AMap_TL_HistoList, AMap_ConstTL_HistoList;
    std::vector<TObject*> AMap_Reco_HistoList, AMap_ConstReco_HistoList;
    std::vector<TObject*> AMap_Ratio_HistoList, AMap_ConstRatio_HistoList;
    std::vector<TObject*> cPart_Sep_AMaps_HistoList, nPart_Sep_AMaps_HistoList_Const;
    std::vector<TObject*> AMaps_HistoList;

    std::vector<std::vector<double>> Loaded_ElectronMomSliceLimits;
    std::vector<std::vector<double>> Loaded_NucleonMomSliceLimits;

    /* Loaded acceptance maps */
    std::vector<std::vector<int>> Loaded_e_AMap;
    std::vector<std::vector<std::vector<int>>> Loaded_e_AMap_Slices_extended;
    std::vector<std::vector<std::vector<int>>> Loaded_e_AMap_Slices;
    std::vector<std::vector<int>> Loaded_p_AMap;
    std::vector<std::vector<std::vector<int>>> Loaded_p_AMap_Slices;
    std::vector<std::vector<int>> Loaded_n_AMap;
    std::vector<std::vector<std::vector<int>>> Loaded_n_AMap_Slices;
    std::vector<std::vector<std::vector<int>>> Loaded_n_AMap_ConstSlices;
    std::vector<std::vector<int>> Loaded_nuc_AMap;
    std::vector<std::vector<std::vector<int>>> Loaded_nuc_AMap_Slices;

    /* Loaded weight maps */
    std::vector<std::vector<std::vector<double>>> Loaded_e_WMap_Slices;
    std::vector<std::vector<std::vector<double>>> Loaded_p_WMap_Slices;
    std::vector<std::vector<int>> Loaded_n_WMap;
    std::vector<std::vector<std::vector<double>>> Loaded_n_WMap_Slices;
    std::vector<std::vector<std::vector<double>>> Loaded_n_WMap_ConstSlices;

    /* Loaded Acceptance maps */
    // TODO: delete these histograms if the .par loading works
    std::vector<TH2*> LoadedElectronAMaps, LoadedProtonAMaps;  // separated AMaps for each bin
    TH2D* LoadedElectronAMaps0;
    TH2D* LoadedProtonAMap;
    TH2D* LoadedNeutronAMap;
    TH2D* LoadedNucleonAMap;

    std::string SName;

    //    bool e_single_slice_test, nuc_single_slice_test;
    bool e_single_slice_test = false;    // keep as false for normal runs! (false by default)
    bool nuc_single_slice_test = false;  // keep as false for normal runs! (false by default)
    std::vector<int> Slices2Test;
};

#endif  // AMAPS_H
