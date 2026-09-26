//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file legacy_uniform_driver.cpp
 * @brief Run archived uniform functions for comparison tests.
 *
 * Purpose:
 *   Give fixed random streams and seeds to the read-only archive without running its cleanup scripts.
 *
 * Workflow:
 *   CTest supplies the settings and reads this program's exit status.
 */

// Run only the archived event functions, never their launch or cleanup scripts.
#include <TFile.h>
#include <TRandom3.h>
#include <TVector3.h>
#include <limits.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>
using namespace std;

#include "legacy/Uniform-sample-generator/Generate_uniform_event.C"
#include "legacy/Uniform-sample-generator/Generate_uniform_event_e_tester.C"
#include "legacy/Uniform-sample-generator/Histograms.cpp"

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Run archived uniform functions for one comparison test.
 *
 * Steps:
 *   Set the random seeds, create the requested events, and save their plots.
 *
 * @param argc Number of executable arguments.
 * @param argv Paths and options supplied by the caller.
 *
 * @return Zero on success; nonzero for an invalid command or failed run.
 */
int main(int argc, char** argv) {
    if (argc != 9) { return 2; }
    const string channel = argv[1], output = argv[2], target = argv[8];
    const double beam = stod(argv[3]);
    const int events = stoi(argv[4]), files = stoi(argv[5]);
    TRandom3 kinematics(stoul(argv[6]));
    ran.SetSeed(stoul(argv[7]));
    filesystem::create_directories(output);
    const bool tester = channel == "tester";
    if (tester) {
        InitHistograms_Tester_e(beam);
    } else {
        InitHistograms(channel == "1e", channel == "ep", channel == "en", beam);
    }
    auto h1 = tester ? TH1_hist_list_Tester_e : channel == "1e" ? TH1_hist_list_1e : channel == "ep" ? TH1_hist_list_ep : TH1_hist_list_en;
    auto h2 = tester ? TH2_hist_list_Tester_e : channel == "1e" ? TH2_hist_list_1e : channel == "ep" ? TH2_hist_list_ep : TH2_hist_list_en;
    for (int i = 1; i <= files; ++i) {
        ofstream out(output + "/legacy_" + to_string(i) + ".txt");
        if (tester) {
            Generate_uniform_event_e_tester(TVector3(0, 0, -3), h1, h2, out, "", "", kinematics, events, 0, 0, 1, 11, beam, beam, 1, mass_e, 5, 40);
        } else if (channel == "1e") {
            Generate_uniform_event(target, h1, h2, out, "", "", kinematics, events, 0, 0, 1, 11, beam, beam, 1, mass_e, 5, 40);
        } else {
            Generate_uniform_event(TString(ConfigBeamE(beam)), target, h1, h2, out, "", "", kinematics, channel == "ep" ? 2212 : 2112, events, 2, 0, 0, 1, 11, beam, beam, 1, mass_e,
                                   channel == "ep" ? mass_p : mass_n, 5, 40, 5, channel == "ep" ? 45 : 35);
        }
    }
    TFile hist((output + "/histograms.root").c_str(), "RECREATE");
    for (auto* h : h1) {
        h->Sumw2();
        h->Write();
    }
    for (auto* h : h2) {
        h->Sumw2();
        h->Write();
    }
}
#pragma endregion
