//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LegacyMonitoring.cpp
 * @brief Implements the archived uniform-sample monitoring contract.
 *
 * Purpose:
 *   Recreate the historical `1e`, `ep`, `en`, and `Tester_e` histogram names,
 *   binning, axes, and correlations using run-local C++ ownership. This module
 *   observes written events; it does not generate, select, or serialize them.
 *
 * Workflow:
 *   1. Select and allocate the archived definitions for one uniform channel.
 *   2. Resolve each definition's symbolic axis keys from every written Event.
 *   3. Write the accumulated histograms to a new ROOT file.
 *   4. Optionally render a multipage PDF and one PNG per histogram.
 *
 * Units and assumptions:
 *   Momentum is in GeV/c, beam energy is in GeV, angles are in degrees, and
 *   vertices are in centimeters. Names, ranges, bin counts, and correlations
 *   are retained because parity tests and existing workflows depend on them.
 *
 * Failure behavior:
 *   Missing event quantities, ROOT output failures, and rendering filesystem
 *   failures are reported with exceptions to the calling generation workflow.
 */

#include "common/LegacyMonitoring.h"

#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TMath.h>
#include <TROOT.h>

#include <stdexcept>
#include <vector>

namespace samples {

// LegacyMonitoring::Impl object -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LegacyMonitoring::Impl object */

/**
 * @struct LegacyMonitoring::Impl
 * @brief Own the selected channel's historical diagnostic definitions.
 *
 * Purpose:
 *   Preserve histogram definitions and axis-key metadata without the archived
 *   implementation's global ROOT state.
 *
 * Lifecycle and ownership:
 *   Construction appends Entry records in historical order. fill() reads them,
 *   save() writes and optionally renders them, and destruction releases every
 *   detached histogram through std::unique_ptr.
 *
 * Invariant:
 *   Each entry owns one initialized histogram and constructor-controlled keys.
 *   Entry order remains stable through filling, writing, and rendering.
 */
struct LegacyMonitoring::Impl {
    // Entry object ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Entry object */

    /**
     * @struct Entry
     * @brief Couple an owned histogram to the event quantities that fill its axes.
     *
     * Creation and use:
     *   Construction associates keys such as `Phi_e`, `P_p`, or `Vz_n` with a
     *   detached ROOT object. fill() resolves those keys from each event.
     *
     * Invariants and units:
     *   An empty y key denotes a TH1. A nonempty y key requires a TH2-compatible
     *   object. Angles are in degrees, momenta in GeV/c, and vertices in cm.
     */
    struct Entry {
        std::unique_ptr<TH1> histogram;  ///< Detached TH1/TH2 object owned by this entry.
        std::string x, y;                ///< Axis quantity keys; empty y means one-dimensional.
    };

#pragma endregion

    std::vector<Entry> entries;  ///< Ordered channel definitions, reused for filling and saving.
};

#pragma endregion

namespace {

// value -----------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* value */

/**
 * @brief Resolve a historical histogram axis quantity.
 *
 * Purpose:
 *   Translate a private `<metric>_<species>` key into one value from a written
 *   electron, proton, or neutron record.
 *
 * Algorithm:
 *   Select the PDG code from the species suffix, find the first matching event
 *   particle, select the metric from the prefix, and convert angles to degrees.
 *
 * @param key Constructor-controlled metric and particle token.
 * @param event Written uniform event supplying the particle values.
 *
 * @return Momentum in GeV/c, an angle in degrees, or a vertex coordinate in cm.
 *
 * @throws std::runtime_error If no supported metric and required particle match the key.
 *
 * @note The private key is assumed to be nonempty and to end in `e`, `p`, or `n`.
 */
double value(const std::string& key, const Event& event) {
    int pid = key.back() == 'e' ? 11 : key.back() == 'p' ? 2212 : 2112;
    for (const auto& p : event.particles) {
        if (p.pid != pid) { continue; }

        auto metric = key.substr(0, key.size() - 2);

        if (metric == "Theta") { return p.momentum.Theta() * TMath::RadToDeg(); }
        if (metric == "Phi") { return p.momentum.Phi() * TMath::RadToDeg(); }
        if (metric == "P") { return p.momentum.Mag(); }
        if (metric == "Vx") { return p.vertex.X(); }
        if (metric == "Vy") { return p.vertex.Y(); }
        if (metric == "Vz") { return p.vertex.Z(); }
    }

    throw std::runtime_error("No histogram quantity " + key);
}

#pragma endregion

}  // namespace

// LegacyMonitoring::LegacyMonitoring ------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LegacyMonitoring::LegacyMonitoring */

/**
 * @brief Build the selected channel historical diagnostic set.
 *
 * Algorithm:
 *   Create an insertion helper that detaches ROOT ownership, select the matching
 *   channel definition, and append its named one- and two-dimensional plots with
 *   their symbolic axis keys. ep/en include both particle-level distributions
 *   and electron-nucleon correlations.
 *
 * @param channel 1e, ep, en or Tester_e diagnostic selection.
 * @param Ebeam Beam energy in GeV used for momentum axes.
 *
 * @post A recognized channel owns its complete empty histogram set in archived order.
 *
 * @note The caller validates channel before construction. An unrecognized value
 *       creates an empty set rather than selecting a fallback definition.
 */
LegacyMonitoring::LegacyMonitoring(const std::string& channel, double Ebeam) : impl_(std::make_unique<Impl>()) {
    auto add = [&](std::unique_ptr<TH1> h, std::string x, std::string y) {
        h->SetDirectory(nullptr);
        impl_->entries.push_back({std::move(h), std::move(x), std::move(y)});
    };

    // Tester electron channel: fixed-vertex tester plots without vertex histograms.
    if (channel == "Tester_e") {
        add(std::make_unique<TH1D>("Theta_e_Tester_e", "#theta_{e} in Tester_e sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_Tester_e", "#phi_{e} in Tester_e sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_Tester_e", "P_{e} in Tester_e sample;P_{e} [GeV/c]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_Tester_e", "#theta_{e} vs. #phi_{e} in Tester_e sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e",
            "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_Tester_e", "#theta_{e} vs. P_{e} in Tester_e sample;P_{e} [GeV/c];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_Tester_e", "#phi_{e} vs. P_{e} in Tester_e sample;P_{e} [GeV/c];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
    }

    // Production electron-only channel: electron kinematics and target vertex.
    if (channel == "1e") {
        add(std::make_unique<TH1D>("Theta_e_1e", "#theta_{e} in (e,e') sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_1e", "#phi_{e} in (e,e') sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_1e", "P_{e} in (e,e') sample;P_{e} [GeV/c]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH1D>("Vx_e_1e", "V_{e,x} of e in (e,e') sample;V_{e,x} [cm]", 100, -5, 5), "Vx_e", "");
        add(std::make_unique<TH1D>("Vy_e_1e", "V_{e,y} of e in (e,e') sample;V_{e,y} [cm]", 100, -5, 5), "Vy_e", "");
        add(std::make_unique<TH1D>("Vz_e_1e", "V_{e,z} of e in (e,e') sample;V_{e,z} [cm]", 100, -5, 5), "Vz_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_1e", "#theta_{e} vs. #phi_{e} in (e,e') sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_1e", "#theta_{e} vs. P_{e} in (e,e') sample;P_{e} [GeV/c];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_1e", "#phi_{e} vs. P_{e} in (e,e') sample;P_{e} [GeV/c];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
    }

    // Electron-proton channel: particle-level plots and electron-proton correlations.
    if (channel == "ep") {
        add(std::make_unique<TH1D>("Theta_e_ep", "#theta_{e} in (e,e'p) sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_ep", "#phi_{e} in (e,e'p) sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_ep", "P_{e} in (e,e'p) sample;P_{e} [GeV/c]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH1D>("Vx_e_ep", "V_{e,x} of e in (e,e'p) sample;V_{e,x} [cm]", 100, -5, 5), "Vx_e", "");
        add(std::make_unique<TH1D>("Vy_e_ep", "V_{e,y} of e in (e,e'p) sample;V_{e,y} [cm]", 100, -5, 5), "Vy_e", "");
        add(std::make_unique<TH1D>("Vz_e_ep", "V_{e,z} of e in (e,e'p) sample;V_{e,z} [cm]", 100, -5, 5), "Vz_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_ep", "#theta_{e} vs. #phi_{e} in (e,e'p) sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_ep", "#theta_{e} vs. P_{e} in (e,e'p) sample;P_{e} [GeV/c];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_ep", "#phi_{e} vs. P_{e} in (e,e'p) sample;P_{e} [GeV/c];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
        add(std::make_unique<TH1D>("Theta_p_ep", "#theta_{p} in (e,e'p) sample;#theta_{p} [#circ]", 100, 0, 50), "Theta_p", "");
        add(std::make_unique<TH1D>("Phi_p_ep", "#phi_{p} in (e,e'p) sample;#phi_{p} [#circ]", 100, -180, 180), "Phi_p", "");
        add(std::make_unique<TH1D>("P_p_ep", "P_{p} in (e,e'p) sample;P_{p} [GeV/c]", 100, 0, Ebeam * 1.1), "P_p", "");
        add(std::make_unique<TH1D>("Vx_p_ep", "V_{p,x} of p in (e,e'p) sample;V_{p,x} [cm]", 100, -5, 5), "Vx_p", "");
        add(std::make_unique<TH1D>("Vy_p_ep", "V_{p,y} of p in (e,e'p) sample;V_{p,y} [cm]", 100, -5, 5), "Vy_p", "");
        add(std::make_unique<TH1D>("Vz_p_ep", "V_{p,z} of p in (e,e'p) sample;V_{p,z} [cm]", 100, -5, 5), "Vz_p", "");
        add(std::make_unique<TH2D>("Theta_p_VS_Phi_p_ep", "#theta_{p} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#theta_{p} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_p", "Theta_p");
        add(std::make_unique<TH2D>("Theta_p_VS_P_p_ep", "#theta_{p} vs. P_{p} in (e,e'p) sample;P_{p} [GeV/c];#theta_{p} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_p", "Theta_p");
        add(std::make_unique<TH2D>("Phi_p_VS_P_p_ep", "#phi_{p} vs. P_{p} in (e,e'p) sample;P_{p} [GeV/c];#phi_{p} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_p", "Phi_p");
        add(std::make_unique<TH2D>("P_e_VS_P_p_ep", "P_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV/c];P_{e} [GeV/c]", 100, 0, Ebeam * 1.1, 100, 0, Ebeam * 1.1), "P_p", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Theta_p_ep", "P_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];P_{e} [GeV/c]", 100, 0, 50, 100, 0, Ebeam * 1.1), "Theta_p", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Phi_p_ep", "P_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];P_{e} [GeV/c]", 100, -180, 180, 100, 0, Ebeam * 1.1), "Phi_p", "P_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_p_ep", "#theta_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV/c];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_p", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Theta_p_ep", "#theta_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];#theta_{e} [#circ]", 100, 0, 50, 100, 0, 50), "Theta_p",
            "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_p_ep", "#theta_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_p", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_p_ep", "#phi_{e} vs. P_{p} in (e,e'p) sample;P_{p} [GeV/c];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_p", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Theta_p_ep", "#phi_{e} vs. #theta_{p} in (e,e'p) sample;#theta_{p} [#circ];#phi_{e} [#circ]", 100, 0, 50, 100, -180, 180), "Theta_p", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Phi_p_ep", "#phi_{e} vs. #phi_{p} in (e,e'p) sample;#phi_{p} [#circ];#phi_{e} [#circ]", 100, -200, 200, 100, -200, 200), "Phi_p", "Phi_e");
    }

    // Electron-neutron channel: particle-level plots and electron-neutron correlations.
    if (channel == "en") {
        add(std::make_unique<TH1D>("Theta_e_en", "#theta_{e} in (e,e'n) sample;#theta_{e} [#circ]", 100, 0, 50), "Theta_e", "");
        add(std::make_unique<TH1D>("Phi_e_en", "#phi_{e} in (e,e'n) sample;#phi_{e} [#circ]", 100, -180, 180), "Phi_e", "");
        add(std::make_unique<TH1D>("P_e_en", "P_{e} in (e,e'n) sample;P_{e} [GeV/c]", 100, 0, Ebeam * 1.1), "P_e", "");
        add(std::make_unique<TH1D>("Vx_e_en", "V_{e,x} of e in (e,e'n) sample;V_{e,x} [cm]", 100, -5, 5), "Vx_e", "");
        add(std::make_unique<TH1D>("Vy_e_en", "V_{e,y} of e in (e,e'n) sample;V_{e,y} [cm]", 100, -5, 5), "Vy_e", "");
        add(std::make_unique<TH1D>("Vz_e_en", "V_{e,z} of e in (e,e'n) sample;V_{e,z} [cm]", 100, -5, 5), "Vz_e", "");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_e_en", "#theta_{e} vs. #phi_{e} in (e,e'n) sample;#phi_{e} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_e", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_e_en", "#theta_{e} vs. P_{e} in (e,e'n) sample;P_{e} [GeV/c];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_e", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_e_en", "#phi_{e} vs. P_{e} in (e,e'n) sample;P_{e} [GeV/c];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_e", "Phi_e");
        add(std::make_unique<TH1D>("Theta_n_en", "#theta_{n} in (e,e'n) sample;#theta_{n} [#circ]", 100, 0, 50), "Theta_n", "");
        add(std::make_unique<TH1D>("Phi_n_en", "#phi_{n} in (e,e'n) sample;#phi_{n} [#circ]", 100, -180, 180), "Phi_n", "");
        add(std::make_unique<TH1D>("P_n_en", "P_{n} in (e,e'n) sample;P_{n} [GeV/c]", 100, 0, Ebeam * 1.1), "P_n", "");
        add(std::make_unique<TH1D>("Vx_n_en", "V_{n,x} of n in (e,e'n) sample;V_{n,x} [cm]", 100, -5, 5), "Vx_n", "");
        add(std::make_unique<TH1D>("Vy_n_en", "V_{n,y} of n in (e,e'n) sample;V_{n,y} [cm]", 100, -5, 5), "Vy_n", "");
        add(std::make_unique<TH1D>("Vz_n_en", "V_{n,z} of n in (e,e'n) sample;V_{n,z} [cm]", 100, -5, 5), "Vz_n", "");
        add(std::make_unique<TH2D>("Theta_n_VS_Phi_n_en", "#theta_{n} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#theta_{n} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_n", "Theta_n");
        add(std::make_unique<TH2D>("Theta_n_VS_P_n_en", "#theta_{n} vs. P_{n} in (e,e'n) sample;P_{n} [GeV/c];#theta_{n} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_n", "Theta_n");
        add(std::make_unique<TH2D>("Phi_n_VS_P_n_en", "#phi_{n} vs. P_{n} in (e,e'n) sample;P_{n} [GeV/c];#phi_{n} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_n", "Phi_n");
        add(std::make_unique<TH2D>("P_e_VS_P_n_en", "P_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV/c];P_{e} [GeV/c]", 100, 0, Ebeam * 1.1, 100, 0, Ebeam * 1.1), "P_n", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Theta_n_en", "P_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];P_{e} [GeV/c]", 100, 0, 50, 100, 0, Ebeam * 1.1), "Theta_n", "P_e");
        add(std::make_unique<TH2D>("P_e_VS_Phi_n_en", "P_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];P_{e} [GeV/c]", 100, -180, 180, 100, 0, Ebeam * 1.1), "Phi_n", "P_e");
        add(std::make_unique<TH2D>("Theta_e_VS_P_n_en", "#theta_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV/c];#theta_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, 0, 50), "P_n", "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Theta_n_en", "#theta_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];#theta_{e} [#circ]", 100, 0, 50, 100, 0, 50), "Theta_n",
            "Theta_e");
        add(std::make_unique<TH2D>("Theta_e_VS_Phi_n_en", "#theta_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#theta_{e} [#circ]", 100, -180, 180, 100, 0, 50), "Phi_n", "Theta_e");
        add(std::make_unique<TH2D>("Phi_e_VS_P_n_en", "#phi_{e} vs. P_{n} in (e,e'n) sample;P_{n} [GeV/c];#phi_{e} [#circ]", 100, 0, Ebeam * 1.1, 100, -180, 180), "P_n", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Theta_n_en", "#phi_{e} vs. #theta_{n} in (e,e'n) sample;#theta_{n} [#circ];#phi_{e} [#circ]", 100, 0, 50, 100, -180, 180), "Theta_n", "Phi_e");
        add(std::make_unique<TH2D>("Phi_e_VS_Phi_n_en", "#phi_{e} vs. #phi_{n} in (e,e'n) sample;#phi_{n} [#circ];#phi_{e} [#circ]", 100, -200, 200, 100, -200, 200), "Phi_n", "Phi_e");
    }
}

#pragma endregion

// LegacyMonitoring destruction ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Histogram destruction */

/**
 * @brief Release the implementation and every detached compatibility histogram.
 *
 * The out-of-line definition lets the public header keep Impl incomplete.
 */
LegacyMonitoring::~LegacyMonitoring() = default;

#pragma endregion

// LegacyMonitoring::fill ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LegacyMonitoring::fill */

/**
 * @brief Fill the historical channel histograms.
 *
 * Algorithm:
 *   Visit entries in archived order. Resolve x for every entry; fill a TH1 when
 *   y is empty, otherwise resolve y and fill the histogram through its TH2 view.
 *
 * @param event Successfully written event containing the configured channel particles.
 *
 * @throws std::runtime_error If a configured quantity cannot be resolved.
 *
 * @note The event is borrowed and unchanged. Earlier entries remain accumulated
 *       if a later entry raises an exception.
 */
void LegacyMonitoring::fill(const Event& event) {
    for (auto& entry : impl_->entries) {
        if (entry.y.empty()) {
            entry.histogram->Fill(value(entry.x, event));
        } else {
            static_cast<TH2*>(entry.histogram.get())->Fill(value(entry.x, event), value(entry.y, event));
        }
    }
}

#pragma endregion

// LegacyMonitoring::save ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LegacyMonitoring::save */

/**
 * @brief Persist historical histograms and optionally render plots.
 *
 * Algorithm:
 *   Create the ROOT file without overwriting an existing result, enable stored
 *   sum-of-weights errors, write every histogram, and close the file. If render
 *   is enabled, enter ROOT batch mode, create `monitoring_plots`, then draw every
 *   entry into a multipage PDF and an individually named PNG.
 *
 * @param path Destination for the new compatibility ROOT file.
 * @param render Whether to create `monitoring_plots/uniform.pdf` and PNG files.
 *
 * @throws std::runtime_error If ROOT cannot create the file or write a histogram.
 * @throws std::filesystem::filesystem_error If the plot directory cannot be created.
 *
 * @note The workflow saves diagnostics before publishing its completion manifest,
 *       so a reported failure prevents the run from being marked complete.
 */
void LegacyMonitoring::save(const std::filesystem::path& path, bool render) {
    TFile out(path.string().c_str(), "CREATE");

    if (out.IsZombie()) { throw std::runtime_error("Cannot create legacy monitoring file"); }

    for (auto& entry : impl_->entries) {
        entry.histogram->Sumw2();
        if (entry.histogram->Write() <= 0) { throw std::runtime_error("Cannot write legacy histogram"); }
    }

    out.Close();

    // Rendering is optional and follows successful numerical ROOT output.
    if (render) {
        gROOT->SetBatch(true);
        const auto directory = path.parent_path() / "monitoring_plots";
        std::filesystem::create_directory(directory);
        const auto pdf = (directory / "uniform.pdf").string();
        TCanvas canvas("sample_monitoring", "Uniform sample monitoring", 1000, 750);
        canvas.Print((pdf + "[").c_str());
        for (auto& entry : impl_->entries) {
            canvas.Clear();
            canvas.SetGrid();
            entry.histogram->Draw(entry.y.empty() ? "hist" : "colz");
            canvas.Print(pdf.c_str());
            canvas.Print((directory / (std::string(entry.histogram->GetName()) + ".png")).string().c_str());
        }
        canvas.Print((pdf + "]").c_str());
    }
}

#pragma endregion

}  // namespace samples
