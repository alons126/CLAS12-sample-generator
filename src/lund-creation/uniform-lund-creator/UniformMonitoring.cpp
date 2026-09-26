//
// Created by Alon Sportes on 19/09/2026.
//

/**
 * @file UniformMonitoring.cpp
 * @brief Creates and saves monitoring plots for uniform samples.
 *
 * Purpose:
 *   Keep the established plot order and style. Add clear FD/CD names for each supported hadron, and
 *   store every histogram once. Use -8 to 5 cm for every Vz plot so all RG-M target positions fit.
 *
 * Workflow:
 *   Create the histograms for one uniform channel -> fill them after each event is written -> apply the
 *   saved axis style -> write one ROOT file -> render the same plots as PDF and PNG files.
 *
 * Failure behavior:
 *   Missing particles, unknown labels, ROOT file errors, and directory errors throw exceptions.
 *   The caller saves the plots before writing the completed LUND run log.
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

// UniformMonitoring::Impl object ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformMonitoring::Impl object */

/**
 * @struct UniformMonitoring::Impl
 * @brief Stores the plots in fill and output order.
 *
 * Each Entry stores one histogram and the particle values used for its axes. An empty y_metric means a
 * one-dimensional histogram. Entry order is also the output order.
 */
struct UniformMonitoring::Impl {
    /** @struct Entry @brief One histogram and the particle values used to fill it. */
    struct Entry {
        std::unique_ptr<TH1> histogram;  ///< ROOT histogram owned only by this Entry.
        std::string x_metric;            ///< `P`, `Theta`, `Phi`, `Vx`, `Vy`, or `Vz`.
        int x_pid;                       ///< PDG identity supplying the x quantity.
        std::string y_metric;            ///< Empty for TH1; otherwise the y-axis metric.
        int y_pid;                       ///< PDG identity supplying the y quantity.
    };

    std::vector<Entry> entries;  ///< Stable fill/write/render order.
};

#pragma endregion

// Private definition helpers --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private definition helpers */

namespace {

/** @struct HadronLabel @brief Stores the ROOT name and displayed title for one regional hadron. */
struct HadronLabel {
    std::string name;   ///< `pFD`, `nCD`, `pipFD`, or `pimCD`.
    std::string title;  ///< `pFD`, `nCD`, `#pi^{+}FD`, or `#pi^{-}CD`.
};

/**
 * @brief Build the ROOT name and TLatex title for one supported hadron and region.
 *
 * @param pid Supported hadron PDG identifier.
 * @param region Detector-region suffix, normally `FD` or `CD`.
 * @return The plain histogram name and formatted title.
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
 * @brief Read one monitored scalar from the first particle with the requested identity.
 *
 * @param event Event to read without changing it.
 * @param pid PDG identifier of the particle to inspect.
 * @param metric Supported quantity name: `P`, `Theta`, `Phi`, `Vx`, `Vy`, or `Vz`.
 * @return Momentum in GeV/c, angle in degrees, or vertex coordinate in cm, as selected by metric.
 * @throws std::runtime_error If the particle is absent or the metric is unsupported.
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

// UniformMonitoring construction ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformMonitoring construction */

UniformMonitoring::UniformMonitoring(std::string sample_label, int hadron_pid, double beam) : impl_(std::make_unique<Impl>()) {
    // Use one Vz display range for every target. It covers the positions of all RG-M target geometries.
    constexpr double vertex_z_min = -8;
    constexpr double vertex_z_max = 5;

    auto one = [&](std::string name, std::string title, double low, double high, std::string metric, int pid) {
        auto histogram = std::make_unique<TH1D>(name.c_str(), title.c_str(), 100, low, high);
        histogram->SetDirectory(nullptr);
        impl_->entries.push_back({std::move(histogram), std::move(metric), pid, "", 0});
    };
    auto two = [&](std::string name, std::string title, double x_low, double x_high, double y_low, double y_high, std::string x_metric, int x_pid, std::string y_metric, int y_pid) {
        auto histogram = std::make_unique<TH2D>(name.c_str(), title.c_str(), 100, x_low, x_high, 100, y_low, y_high);
        histogram->SetDirectory(nullptr);
        impl_->entries.push_back({std::move(histogram), std::move(x_metric), x_pid, std::move(y_metric), y_pid});
    };

    const bool tester = sample_label == "electron-tester";
    const bool electron_only = sample_label == "1e" || tester;
    const std::string suffix = tester ? "Tester_e" : sample_label;
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

    const std::string region = sample_label.size() >= 2 && sample_label.compare(sample_label.size() - 2, 2, "CD") == 0 ? "CD" : "FD";
    const auto hadron = hadronLabel(hadron_pid, region);
    // ROOT shows the object name in its statistics box. Include the hadron and detector region.
    const std::string& channel = sample_label;
    const std::string context = "(e,e'" + hadron.title + ") sample";
    const double theta_high = region == "CD" ? 150 : 50;

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

// Event filling ---------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Event filling */

void UniformMonitoring::fill(const Event& event) {
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

// Output and rendering --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Output and rendering */

void UniformMonitoring::save(const std::filesystem::path& path, const std::filesystem::path& plot_directory, const std::string& pdf_name) {
    // Apply the axis style before writing so saved and drawn plots look the same.
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

    TFile output(path.string().c_str(), "CREATE");
    if (output.IsZombie()) { throw std::runtime_error("Cannot create uniform monitoring ROOT file"); }

    for (auto& entry : impl_->entries) {
        if (entry.histogram->Write() <= 0) { throw std::runtime_error("Cannot write uniform monitoring histogram"); }
    }
    output.Close();

    gROOT->SetBatch(true);
    std::filesystem::create_directories(plot_directory);
    const auto pdf = (plot_directory / pdf_name).string();
    TCanvas canvas("canvas", "canvas", 1000, 750);
    canvas.SetGrid();
    canvas.SetBottomMargin(0.14);
    canvas.SetLeftMargin(0.16);
    canvas.SetRightMargin(0.12);
    canvas.cd();
    canvas.Print((pdf + "[").c_str());

    std::size_t index = 0;
    for (auto& entry : impl_->entries) {
        auto* histogram = entry.histogram.get();
        histogram->Draw(entry.y_metric.empty() ? "" : "colz");
        canvas.SaveAs((plot_directory / (std::to_string(++index) + "_" + histogram->GetName() + ".png")).string().c_str());
        canvas.Print(pdf.c_str());
        canvas.Clear();
    }
    canvas.Print((pdf + "]").c_str());
}

#pragma endregion

}  // namespace samples
