//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LegacyMonitoring.h
 * @brief Declares legacy-compatible uniform-sample monitoring.
 *
 * Purpose:
 *   Reproduce the archived uniform generator's channel-specific histogram names,
 *   binning, axis definitions and correlations without reintroducing its global
 *   ROOT objects. This interface supports output-parity checks and familiar
 *   operational plots; it does not generate, select or serialize events.
 *
 * Workflow:
 *   1. Construct one monitor with a uniform channel and beam energy.
 *   2. Pass every successfully written Event to fill().
 *   3. Call save() after generation to create the compatibility ROOT file.
 *   4. Optionally render the same histograms as PDF and PNG plots.
 *
 * Relationship to Monitoring:
 *   Monitoring supplies a general per-PDG view shared by uniform and physical
 *   sources. LegacyMonitoring is an additional uniform-only view whose names,
 *   ranges and cross-particle correlations follow the archived implementation.
 *
 * Units and failure behavior:
 *   Beam energy is in GeV, particle momentum is in GeV/c, angles are in
 *   degrees, and vertices are in centimeters. Missing channel particles or
 *   quantities fail during fill(); output creation or writing fails during save().
 */

#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include "lund/Event.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// LegacyMonitoring object -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LegacyMonitoring object */

/**
 * @class LegacyMonitoring
 * @brief Maintained channel diagnostics matching the archived numerical definitions.
 *
 * Purpose:
 *   Preserve the monitoring contract expected for the historical `1e`, `ep`,
 *   `en`, and `Tester_e` uniform modes while keeping all state local to one run.
 *
 * Usage:
 *   Select the channel and beam scale at construction, call fill() only for
 *   successfully written events of that channel, and call save() once the event
 *   loop completes. Rendering is an optional final presentation step.
 *
 * State and ownership:
 *   The private implementation owns an ordered collection of detached ROOT
 *   histograms and the symbolic event quantities used for their axes. The pImpl
 *   boundary hides ROOT types from this header. The class is non-copyable because
 *   it exclusively owns that implementation.
 *
 * Invariants:
 *   Events supplied to fill() must contain the particles required by the chosen
 *   channel: an electron for `1e` and `Tester_e`, plus a proton for `ep` or a
 *   neutron for `en`. Histogram contents accumulate sequentially for one run.
 */
class LegacyMonitoring {
   public:
    /**
     * @brief Build the archived histogram set for one uniform channel.
     *
     * @param channel One of `1e`, `ep`, `en`, or `Tester_e`.
     * @param beam Beam energy in GeV, used to size momentum-axis ranges.
     *
     * @note Histogram objects are allocated during construction and detached
     *       from ROOT directory ownership.
     */
    LegacyMonitoring(const std::string& channel, double beam);

    /**
     * @brief Release the implementation and all detached histograms it owns.
     *
     * The destructor is defined out of line so Impl remains incomplete here.
     */
    ~LegacyMonitoring();

    /**
     * @brief Add one written uniform event to every configured channel histogram.
     *
     * Symbolic axis keys select electron, proton, or neutron momentum, angles,
     * and vertex coordinates from the event. The event is borrowed for this call
     * and is neither retained nor modified.
     *
     * @param event Successfully written event matching the constructed channel.
     *
     * @throws std::runtime_error If a required particle or axis quantity cannot be resolved.
     */
    void fill(const Event& event);

    /**
     * @brief Persist compatibility histograms and optionally render visualizations.
     *
     * The ROOT file is created at path. When render is true, the implementation creates the selected
     * plot directory containing one named multipage PDF and numbered PNGs in historical histogram order.
     *
     * @param path Destination for the new compatibility ROOT file.
     * @param render Whether to render PDF and PNG plots after writing ROOT data.
     * @param plot_directory Rendering destination. An empty path selects `monitoring_plots` beside path.
     * @param pdf_name Multipage PDF filename inside plot_directory.
     *
     * @throws std::runtime_error If ROOT cannot create the file or write a histogram.
     * @throws std::filesystem::filesystem_error If the rendering directory cannot be created.
     */
    void save(const std::filesystem::path& path, bool render, const std::filesystem::path& plot_directory = {}, const std::string& pdf_name = "uniform.pdf");

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /**
     * @struct Impl
     * @brief ROOT-backed compatibility definitions declared in the implementation.
     *
     * Impl owns the ordered histogram entries and their symbolic x/y quantity
     * keys. Keeping it private prevents ROOT histogram types from entering the
     * public interface.
     */
    struct Impl;

    std::unique_ptr<Impl> impl_;  ///< Exclusive run-long ownership of compatibility state.
};

#pragma endregion

#pragma endregion

}  // namespace samples
