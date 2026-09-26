//
// Created by Alon Sportes on 19/09/2026.
//

/**
 * @file check_monitoring.cpp
 * @brief Check saved uniform-monitoring names, titles, axes, and text style.
 *
 * Purpose:
 *   Test the hadron region names and ROOT plot style.
 *
 * Workflow:
 *   Open the ROOT file -> find one histogram -> check its text and style -> report success or failure.
 */

#include <TFile.h>
#include <TH1.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */

/**
 * @brief Check one representative histogram in a generated monitoring ROOT file.
 * @param argc Six arguments: program, ROOT file, object name, title, x-axis title, expected key count.
 * @param argv Argument storage owned by the process runtime.
 * @return Zero when every check passes; nonzero otherwise.
 */
int main(int argc, char** argv) {
    if (argc != 6) { return 2; }
    try {
        TFile file(argv[1]);
        if (file.IsZombie()) { throw std::runtime_error("Cannot open monitoring ROOT file"); }
        if (file.GetNkeys() != std::stoi(argv[5])) { throw std::runtime_error("Unexpected monitoring histogram count"); }
        auto* histogram = dynamic_cast<TH1*>(file.Get(argv[2]));
        if (!histogram) { throw std::runtime_error("Missing monitoring histogram: " + std::string(argv[2])); }
        if (std::string(histogram->GetTitle()) != argv[3] || std::string(histogram->GetXaxis()->GetTitle()) != argv[4]) { throw std::runtime_error("Monitoring title mismatch"); }
        if (!histogram->GetXaxis()->GetCenterTitle() || !histogram->GetYaxis()->GetCenterTitle() || std::abs(histogram->GetXaxis()->GetTitleSize() - 0.06) > 1e-6 ||
            std::abs(histogram->GetXaxis()->GetLabelSize() - 0.0425) > 1e-6 || std::string(histogram->GetYaxis()->GetTitle()) != "Number of events") {
            throw std::runtime_error("Monitoring axis-style mismatch");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#pragma endregion
