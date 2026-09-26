//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file compare_histograms.cpp
 * @brief Compare numerical ROOT histograms.
 *
 * Purpose:
 *   Compare the stored histogram values instead of the raw ROOT file bytes.
 *
 * Workflow:
 *   Read the archived and maintained ROOT files -> compare each histogram -> report the first
 *   difference. An explicit option allows the corrected vertex-z display range.
 *
 * CLI options:
 *   EXPECTED ACTUAL                     Compare every histogram exactly.
 *   EXPECTED ACTUAL --allow-corrected-vz
 *                                       Allow `Vz_e_1e` to use the maintained -8 to 5 cm range
 *                                       instead of the archived, incorrect -5 to 5 cm range.
 */

#include <TFile.h>
#include <TH1.h>
#include <TKey.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Compare numerical ROOT histograms.
 *
 * Steps:
 *   Check names, axes, entries, bin values, and errors.
 *
 * @param argc Number of executable arguments. Three arguments run an exact comparison; four also
 *             require the `--allow-corrected-vz` option.
 * @param argv Expected ROOT file, actual ROOT file, and the optional corrected-range flag.
 *
 * @return Zero when all checks pass; nonzero otherwise.
 */
int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) { return 2; }
    const bool allow_corrected_vz = argc == 4 && std::string(argv[3]) == "--allow-corrected-vz";
    if (argc == 4 && !allow_corrected_vz) { return 2; }

    try {
        TFile expected(argv[1]), actual(argv[2]);
        if (expected.IsZombie() || actual.IsZombie()) { throw std::runtime_error("Missing ROOT histogram file"); }
        if (expected.GetNkeys() != actual.GetNkeys()) { throw std::runtime_error("Different histogram count"); }
        TIter next(expected.GetListOfKeys());
        while (auto* key = static_cast<TKey*>(next())) {
            auto* old = dynamic_cast<TH1*>(expected.Get(key->GetName()));
            auto* now = dynamic_cast<TH1*>(actual.Get(key->GetName()));
            if (!old || !now || old->GetNcells() != now->GetNcells() || old->GetEntries() != now->GetEntries()) { throw std::runtime_error(key->GetName()); }

            // The old range put some RG-M argon vertices in the underflow bin. The LUND comparison
            // checks the vertex values, so this test permits only the corrected plot range.
            if (allow_corrected_vz && std::string(key->GetName()) == "Vz_e_1e") {
                if (old->GetXaxis()->GetNbins() != 100 || old->GetXaxis()->GetXmin() != -5 || old->GetXaxis()->GetXmax() != 5 || now->GetXaxis()->GetNbins() != 100 ||
                    now->GetXaxis()->GetXmin() != -8 || now->GetXaxis()->GetXmax() != 5) {
                    throw std::runtime_error("Vz_e_1e: corrected axis mismatch");
                }
                continue;
            }

            for (int bin = 0; bin < old->GetNcells(); ++bin) {
                if (old->GetBinContent(bin) != now->GetBinContent(bin) || std::abs(old->GetBinError(bin) - now->GetBinError(bin)) > 1e-12) {
                    throw std::runtime_error(std::string(key->GetName()) + ": bin mismatch");
                }
            }
            for (int axis = 0; axis < 2; ++axis) {
                auto* a = axis ? old->GetYaxis() : old->GetXaxis();
                auto* b = axis ? now->GetYaxis() : now->GetXaxis();
                if (a->GetNbins() != b->GetNbins() || a->GetXmin() != b->GetXmin() || a->GetXmax() != b->GetXmax()) { throw std::runtime_error("Axis mismatch"); }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
#pragma endregion
