//
// Created by Alon Sportes on 19/09/2026.
//

/**
 * @file UniformMonitoring.cpp
 * @brief Builds, fills, saves, and draws the plots used to check uniform LUND events.
 *
 * Purpose:
 *   Show the particle values produced by the uniform LUND creator so the user can spot an incorrect range,
 *   missing particle, or unexpected relationship. Each quantity is stored in one ROOT histogram. Hadron
 *   plot names include FD or CD so the detector region remains clear. Every z-position plot covers -8
 *   through 5 centimeters so all supported RG-M vertex positions fit.
 *
 * Execution flow:
 *   The constructor creates the plot set for one sample. addEventToHistograms() finds the required
 *   particles in each successfully written event and adds their momentum, angles, or position to the plots.
 *   saveHistogramsAndRenderPlots() applies one axis style, writes all histograms to a ROOT file, and draws
 *   the same plots in the same order as a multipage PDF and numbered PNG images.
 *
 * Failure behavior:
 *   A missing required particle, unsupported hadron, ROOT output problem, or directory problem reports an
 *   error. The uniform LUND creator saves these plots before publishing the manifest that marks the run
 *   complete.
 */

#include "uniform-lund-creator/UniformMonitoring.h"

#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2.h>
#include <TH2D.h>
#include <TMath.h>
#include <TROOT.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace samples {

// Stored histograms and fill instructions -------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Stored histograms and fill instructions */

/**
 * @struct UniformMonitoring::Impl
 * @brief Owns every histogram and records which particle value fills each axis.
 *
 * Each Entry connects one histogram axis to a particle and quantity. For example, an entry can fill its
 * x-axis with electron momentum or hadron phi. An empty y_metric means the plot has one value axis.
 * Otherwise, it is a two-value plot. The order of entries is also the order used in the ROOT file, PDF,
 * and numbered PNG filenames.
 */
struct UniformMonitoring::Impl {
    /**
     * @struct Entry
     * @brief Stores one histogram and identifies the value used for each axis.
     *
     * x_metric and y_metric name a momentum, angle, or position coordinate. x_pid and y_pid identify the
     * particles that supply those values. Entry owns its histogram for the complete monitoring lifetime.
     */
    struct Entry {
        std::unique_ptr<TH1> histogram;  ///< ROOT histogram owned by this entry.
        std::string x_metric;            ///< X value: `P`, `Theta`, `Phi`, `Vx`, `Vy`, or `Vz`.
        int x_pid;                       ///< Standard PDG integer for the particle supplying the x value.
        std::string y_metric;            ///< Empty for a one-value plot; otherwise the y value name.
        int y_pid;                       ///< Standard PDG integer for the particle supplying the y value.
    };

    std::vector<Entry> entries;  ///< Plot records in their fixed fill, save, and drawing order.
};

#pragma endregion

// Private plot helpers --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private plot helpers */

/**
 * @namespace samples::<anonymous>
 * @brief Keeps plot-building helpers available only inside this source file.
 */
namespace {

/**
 * @struct HadronLabel
 * @brief Stores the plain object name and formatted plot title for one hadron and detector region.
 *
 * ROOT object names use text such as `pipFD`. Displayed titles use ROOT's math-like form, such as
 * `#pi^{+}FD`, so the drawn plot shows the pion symbol and charge.
 */
struct HadronLabel {
    std::string name;   ///< Plain ROOT object text such as `pFD`, `nCD`, `pipFD`, or `pimCD`.
    std::string title;  ///< Text ROOT draws, such as `pFD`, `nCD`, `#pi^{+}FD`, or `#pi^{-}CD`.
};

/**
 * @brief Build the object-name text and displayed title for one hadron and detector region.
 * @param pid Standard PDG integer for a supported proton, neutron, positive pion, or negative pion.
 * @param region Detector suffix, normally `FD` for forward or `CD` for central.
 * @return Plain text for the ROOT object name and formatted text for the drawn title.
 * @throws std::runtime_error If pid does not identify a supported uniform-sample hadron.
 */
HadronLabel hadronLabel(int pid, const std::string& region) {
    switch (pid) {
        case constants::proton_pdg:
            return {"p" + region, "p" + region};
        case constants::neutron_pdg:
            return {"n" + region, "n" + region};
        case constants::pi_plus_pdg:
            return {"pip" + region, "#pi^{+}" + region};
        case constants::pi_minus_pdg:
            return {"pim" + region, "#pi^{-}" + region};
        default:
            throw std::runtime_error("Unsupported uniform-monitoring hadron PDG: " + std::to_string(pid));
    }
}

/**
 * @brief Read one momentum, angle, or position value from the requested particle.
 * @param event Event to search without changing it.
 * @param pid Standard PDG integer for the particle to find.
 * @param metric Value to return: `P` for momentum size, `Theta` for the angle from the beam direction,
 *               `Phi` for the angle around the beam, or `Vx`, `Vy`, and `Vz` for position coordinates.
 * @return Momentum in GeV/c, an angle in degrees, or a position coordinate in centimeters.
 * @throws std::runtime_error If the event lacks that particle or metric is not one of the supported names.
 */
double quantity(const Event& event, int pid, const std::string& metric) {
    for (const auto& particle : event.particles) {
        if (particle.pid != pid) { continue; }
        if (metric == "P") { return particle.momentum.Mag(); }
        if (metric == "Theta") { return particle.momentum.Theta() * TMath::RadToDeg(); }
        if (metric == "Phi") { return particle.momentum.Phi() * TMath::RadToDeg(); }
        if (metric == "Vx") { return particle.vertex.X(); }
        if (metric == "Vy") { return particle.vertex.Y(); }
        if (metric == "Vz") { return particle.vertex.Z(); }
    }

    throw std::runtime_error("Missing particle/quantity in uniform monitoring: " + metric + ", PDG " + std::to_string(pid));
}

}  // namespace

#pragma endregion

// Creating the plot set -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Creating the plot set */

UniformMonitoring::UniformMonitoring(std::string sample_label, int hadron_pid, double beam) : impl_(std::make_unique<Impl>()) {
    // Use the same z-position range for every target so plots from different runs can be compared directly.
    // This range contains all supported RG-M vertex positions.
    constexpr double vertex_z_min = -8;
    constexpr double vertex_z_max = 5;

    // Add one plot with 100 bins. SetDirectory(nullptr) keeps this object as the owner instead of giving
    // ownership to whichever ROOT file happens to be active.
    auto one = [&](std::string name, std::string title, double low, double high, std::string metric, int pid) {
        auto histogram = std::make_unique<TH1D>(name.c_str(), title.c_str(), 100, low, high);
        histogram->SetDirectory(nullptr);
        impl_->entries.push_back({std::move(histogram), std::move(metric), pid, "", 0});
    };

    // Add one two-value plot with 100 bins along each axis and record the value used for each axis.
    auto two = [&](std::string name, std::string title, double x_low, double x_high, double y_low, double y_high, std::string x_metric, int x_pid, std::string y_metric, int y_pid) {
        auto histogram = std::make_unique<TH2D>(name.c_str(), title.c_str(), 100, x_low, x_high, 100, y_low, y_high);
        histogram->SetDirectory(nullptr);
        impl_->entries.push_back({std::move(histogram), std::move(x_metric), x_pid, std::move(y_metric), y_pid});
    };

    const bool tester = (sample_label == "electron-tester");
    const bool electron_only = ((sample_label == "1e") || tester);
    const std::string suffix = tester ? "Tester_e" : sample_label;

    // Electron-only samples need plots for the electron. The tester does not include position plots
    // because its purpose is the angular scan used to choose the trigger-electron angle.
    if (electron_only) {
        const std::string context = tester ? "Tester_e sample" : "(e,e') sample";
        one("Theta_e_" + suffix, "#theta_{e} in " + context + ";#theta_{e} [#circ]", 0, 50, "Theta", constants::electron_pdg);
        one("Phi_e_" + suffix, "#phi_{e} in " + context + ";#phi_{e} [#circ]", -180, 180, "Phi", constants::electron_pdg);
        one("P_e_" + suffix, "P_{e} in " + context + ";P_{e} [GeV]", 0, beam * 1.1, "P", constants::electron_pdg);
        if (!tester) {
            one("Vx_e_1e", "V_{e,x} of e in (e,e') sample;V_{e,x} [cm]", -5, 5, "Vx", constants::electron_pdg);
            one("Vy_e_1e", "V_{e,y} of e in (e,e') sample;V_{e,y} [cm]", -5, 5, "Vy", constants::electron_pdg);
            one("Vz_e_1e", "V_{e,z} of e in (e,e') sample;V_{e,z} [cm]", vertex_z_min, vertex_z_max, "Vz", constants::electron_pdg);
        }

        two("Theta_e_VS_Phi_e_" + suffix, "#theta_{e} vs. #phi_{e} in " + context + ";#phi_{e} [#circ];#theta_{e} [#circ]", -180, 180, 0, 50, "Phi", constants::electron_pdg, "Theta",
            constants::electron_pdg);
        two("Theta_e_VS_P_e_" + suffix, "#theta_{e} vs. P_{e} in " + context + ";P_{e} [GeV];#theta_{e} [#circ]", 0, beam * 1.1, 0, 50, "P", constants::electron_pdg, "Theta",
            constants::electron_pdg);
        two("Phi_e_VS_P_e_" + suffix, "#phi_{e} vs. P_{e} in " + context + ";P_{e} [GeV];#phi_{e} [#circ]", 0, beam * 1.1, -180, 180, "P", constants::electron_pdg, "Phi",
            constants::electron_pdg);
        return;
    }

    const std::string region = ((sample_label.size() >= 2) && (sample_label.compare(sample_label.size() - 2, 2, "CD") == 0)) ? "CD" : "FD";
    const auto hadron = hadronLabel(hadron_pid, region);

    // ROOT can display the object name on a plot. Include the hadron and detector region so saved plots
    // remain identifiable on their own.
    const std::string& channel = sample_label;
    const std::string context = "(e,e'" + hadron.title + ") sample";
    const double theta_high = (region == "CD") ? 150 : 50;

    // The first group checks the electron alone.
    auto electron_one = [&](const std::string& metric, const std::string& symbol, double low, double high, const std::string& unit) {
        one(metric + "_e_" + channel, symbol + "_{e} in " + context + ";" + symbol + "_{e} " + unit, low, high, metric, constants::electron_pdg);
    };
    electron_one("Theta", "#theta", 0, 50, "[#circ]");
    electron_one("Phi", "#phi", -180, 180, "[#circ]");
    electron_one("P", "P", 0, beam * 1.1, "[GeV]");
    one("Vx_e_" + channel, "V_{e,x} of e in " + context + ";V_{e,x} [cm]", -5, 5, "Vx", constants::electron_pdg);
    one("Vy_e_" + channel, "V_{e,y} of e in " + context + ";V_{e,y} [cm]", -5, 5, "Vy", constants::electron_pdg);
    one("Vz_e_" + channel, "V_{e,z} of e in " + context + ";V_{e,z} [cm]", vertex_z_min, vertex_z_max, "Vz", constants::electron_pdg);
    two("Theta_e_VS_Phi_e_" + channel, "#theta_{e} vs. #phi_{e} in " + context + ";#phi_{e} [#circ];#theta_{e} [#circ]", -180, 180, 0, 50, "Phi", constants::electron_pdg, "Theta",
        constants::electron_pdg);
    two("Theta_e_VS_P_e_" + channel, "#theta_{e} vs. P_{e} in " + context + ";P_{e} [GeV];#theta_{e} [#circ]", 0, beam * 1.1, 0, 50, "P", constants::electron_pdg, "Theta",
        constants::electron_pdg);
    two("Phi_e_VS_P_e_" + channel, "#phi_{e} vs. P_{e} in " + context + ";P_{e} [GeV];#phi_{e} [#circ]", 0, beam * 1.1, -180, 180, "P", constants::electron_pdg, "Phi",
        constants::electron_pdg);

    const std::string& h = hadron.name;
    const std::string& ht = hadron.title;

    // The second group checks the hadron alone.
    one("Theta_" + h + "_" + channel, "#theta_{" + ht + "} in " + context + ";#theta_{" + ht + "} [#circ]", 0, theta_high, "Theta", hadron_pid);
    one("Phi_" + h + "_" + channel, "#phi_{" + ht + "} in " + context + ";#phi_{" + ht + "} [#circ]", -180, 180, "Phi", hadron_pid);
    one("P_" + h + "_" + channel, "P_{" + ht + "} in " + context + ";P_{" + ht + "} [GeV]", 0, beam * 1.1, "P", hadron_pid);
    one("Vx_" + h + "_" + channel, "V_{" + ht + ",x} of " + ht + " in " + context + ";V_{" + ht + ",x} [cm]", -5, 5, "Vx", hadron_pid);
    one("Vy_" + h + "_" + channel, "V_{" + ht + ",y} of " + ht + " in " + context + ";V_{" + ht + ",y} [cm]", -5, 5, "Vy", hadron_pid);
    one("Vz_" + h + "_" + channel, "V_{" + ht + ",z} of " + ht + " in " + context + ";V_{" + ht + ",z} [cm]", vertex_z_min, vertex_z_max, "Vz", hadron_pid);
    two("Theta_" + h + "_VS_Phi_" + h + "_" + channel, "#theta_{" + ht + "} vs. #phi_{" + ht + "} in " + context + ";#phi_{" + ht + "} [#circ];#theta_{" + ht + "} [#circ]", -180, 180, 0,
        theta_high, "Phi", hadron_pid, "Theta", hadron_pid);
    two("Theta_" + h + "_VS_P_" + h + "_" + channel, "#theta_{" + ht + "} vs. P_{" + ht + "} in " + context + ";P_{" + ht + "} [GeV];#theta_{" + ht + "} [#circ]", 0, beam * 1.1, 0,
        theta_high, "P", hadron_pid, "Theta", hadron_pid);
    two("Phi_" + h + "_VS_P_" + h + "_" + channel, "#phi_{" + ht + "} vs. P_{" + ht + "} in " + context + ";P_{" + ht + "} [GeV];#phi_{" + ht + "} [#circ]", 0, beam * 1.1, -180, 180, "P",
        hadron_pid, "Phi", hadron_pid);

    // The final group shows relationships between electron and hadron values.
    two("P_e_VS_P_" + h + "_" + channel, "P_{e} vs. P_{" + ht + "} in " + context + ";P_{" + ht + "} [GeV];P_{e} [GeV]", 0, beam * 1.1, 0, beam * 1.1, "P", hadron_pid, "P",
        constants::electron_pdg);
    two("P_e_VS_Theta_" + h + "_" + channel, "P_{e} vs. #theta_{" + ht + "} in " + context + ";#theta_{" + ht + "} [#circ];P_{e} [GeV]", 0, theta_high, 0, beam * 1.1, "Theta", hadron_pid,
        "P", constants::electron_pdg);
    two("P_e_VS_Phi_" + h + "_" + channel, "P_{e} vs. #phi_{" + ht + "} in " + context + ";#phi_{" + ht + "} [#circ];P_{e} [GeV]", -180, 180, 0, beam * 1.1, "Phi", hadron_pid, "P",
        constants::electron_pdg);
    two("Theta_e_VS_P_" + h + "_" + channel, "#theta_{e} vs. P_{" + ht + "} in " + context + ";P_{" + ht + "} [GeV];#theta_{e} [#circ]", 0, beam * 1.1, 0, 50, "P", hadron_pid, "Theta",
        constants::electron_pdg);
    two("Theta_e_VS_Theta_" + h + "_" + channel, "#theta_{e} vs. #theta_{" + ht + "} in " + context + ";#theta_{" + ht + "} [#circ];#theta_{e} [#circ]", 0, theta_high, 0, 50, "Theta",
        hadron_pid, "Theta", constants::electron_pdg);
    two("Theta_e_VS_Phi_" + h + "_" + channel, "#theta_{e} vs. #phi_{" + ht + "} in " + context + ";#phi_{" + ht + "} [#circ];#theta_{e} [#circ]", -180, 180, 0, 50, "Phi", hadron_pid,
        "Theta", constants::electron_pdg);
    two("Phi_e_VS_P_" + h + "_" + channel, "#phi_{e} vs. P_{" + ht + "} in " + context + ";P_{" + ht + "} [GeV];#phi_{e} [#circ]", 0, beam * 1.1, -180, 180, "P", hadron_pid, "Phi",
        constants::electron_pdg);
    two("Phi_e_VS_Theta_" + h + "_" + channel, "#phi_{e} vs. #theta_{" + ht + "} in " + context + ";#theta_{" + ht + "} [#circ];#phi_{e} [#circ]", 0, theta_high, -180, 180, "Theta",
        hadron_pid, "Phi", constants::electron_pdg);
    two("Phi_e_VS_Phi_" + h + "_" + channel, "#phi_{e} vs. #phi_{" + ht + "} in " + context + ";#phi_{" + ht + "} [#circ];#phi_{e} [#circ]", -200, 200, -200, 200, "Phi", hadron_pid, "Phi",
        constants::electron_pdg);
}

UniformMonitoring::~UniformMonitoring() = default;

#pragma endregion

// Adding one event to the plots -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Adding one event to the plots */
void UniformMonitoring::addEventToHistograms(const Event& event) {
    // Read the x value requested by every entry. A missing y value marks a one-value histogram. Otherwise,
    // ROOT's two-value histogram receives both values.
    for (auto& entry : impl_->entries) {
        const double x = quantity(event, entry.x_pid, entry.x_metric);
        if (entry.y_metric.empty()) {
            entry.histogram->Fill(x);
        } else {
            static_cast<TH2*>(entry.histogram.get())->Fill(x, quantity(event, entry.y_pid, entry.y_metric));
        }
    }
}
#pragma endregion

// Saving and drawing the plots ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Saving and drawing the plots */
void UniformMonitoring::saveHistogramsAndRenderPlots(const std::filesystem::path& path, const std::filesystem::path& plot_directory, const std::string& pdf_name) {
    // Apply one axis style before writing so a histogram opened from the ROOT file looks like the rendered
    // PDF and PNG version. Sumw2() stores information ROOT uses to calculate bin uncertainties.
    for (auto& entry : impl_->entries) {
        auto* histogram = entry.histogram.get();
        histogram->Sumw2();
        histogram->GetXaxis()->CenterTitle();
        histogram->GetXaxis()->SetTitleSize(0.06);
        histogram->GetXaxis()->SetLabelSize(0.0425);
        histogram->GetYaxis()->CenterTitle();
        histogram->GetYaxis()->SetTitleSize(0.06);
        histogram->GetYaxis()->SetLabelSize(0.0425);
        if (entry.y_metric.empty()) { histogram->GetYaxis()->SetTitle("Number of events"); }
    }

    // CREATE refuses to replace an existing ROOT file. This protects monitoring output if the expected run
    // directory replacement did not happen.
    TFile output(path.string().c_str(), "CREATE");
    if (output.IsZombie()) { throw std::runtime_error("Cannot create uniform monitoring ROOT file"); }

    for (auto& entry : impl_->entries) {
        if (entry.histogram->Write() <= 0) { throw std::runtime_error("Cannot write uniform monitoring histogram"); }
    }

    output.Close();

    // Batch mode lets ROOT draw files without opening graphical windows, which also works in batch jobs.
    // Create one canvas with the same grid and margins for every plot.
    gROOT->SetBatch(true);
    std::filesystem::create_directories(plot_directory);
    const auto pdf = (plot_directory / pdf_name).string();
    TCanvas canvas("canvas", "canvas", 1000, 750);
    canvas.SetGrid();
    canvas.SetBottomMargin(0.14);
    canvas.SetLeftMargin(0.16);
    canvas.SetRightMargin(0.12);
    canvas.cd();

    // Printing the PDF name with `[` opens one multipage PDF. Each loop iteration adds a page, and `]`
    // closes the file after the final page.
    canvas.Print((pdf + "[").c_str());

    std::size_t index = 0;
    for (auto& entry : impl_->entries) {
        auto* histogram = entry.histogram.get();

        // Use a color map for two-value plots. Save a numbered PNG before adding the same image to the PDF,
        // then clear the canvas so the next plot starts empty.
        histogram->Draw(entry.y_metric.empty() ? "" : "colz");
        canvas.SaveAs((plot_directory / (std::to_string(++index) + "_" + histogram->GetName() + ".png")).string().c_str());
        canvas.Print(pdf.c_str());
        canvas.Clear();
    }

    canvas.Print((pdf + "]").c_str());
}
#pragma endregion

}  // namespace samples
