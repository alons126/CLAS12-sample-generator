//
// Created by Alon Sportes on 19/09/2026.
//

/**
 * @file UniformMonitoring.h
 * @brief Declares the single uniform-sample monitoring implementation.
 *
 * Purpose:
 *   Keep every uniform monitoring histogram in one ROOT file while preserving the archived histogram
 *   layout and rendering style. Physical conversion does not use this class or create monitoring plots.
 *
 * Workflow:
 *   Construct from the resolved uniform sample identity -> fill after each written event -> save one
 *   ROOT file -> render the same ordered histograms as a PDF and numbered PNG files.
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "lund/Event.h"

namespace samples {

// UniformMonitoring object ---------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformMonitoring object */

/**
 * @class UniformMonitoring
 * @brief Own the legacy-style histogram family for one uniform channel.
 *
 * Purpose:
 *   Provide one monitoring contract for 1e, electron-tester, and every supported electron-hadron
 *   region/species combination. Electron-hadron labels include FD or CD in both ROOT names and titles.
 *
 * Ownership and lifetime:
 *   The private implementation exclusively owns detached ROOT histograms for one generation run. The
 *   object is non-copyable and must remain alive from before the event loop through save().
 *
 * Invariants:
 *   sample_label is one resolved uniform label. Electron-hadron events contain the configured hadron
 *   after the electron. Momentum is GeV, angles are degrees, and vertices are centimeters in plots.
 */
class UniformMonitoring {
   public:
    /**
     * @brief Allocate the complete ordered histogram set for one uniform sample.
     * @param sample_label Resolved label such as `1e`, `electron-tester`, `epFD`, or `epipCD`.
     * @param hadron_pid Configured hadron PDG code; ignored for electron-only channels.
     * @param beam Beam energy in GeV, used for momentum-axis limits.
     */
    UniformMonitoring(std::string sample_label, int hadron_pid, double beam);

    /** @brief Release the private implementation and its detached ROOT histograms. */
    ~UniformMonitoring();

    /**
     * @brief Fill every configured histogram from one successfully written event.
     * @param event Borrowed uniform event; it is neither retained nor modified.
     * @throws std::runtime_error If the event lacks a required electron or hadron.
     */
    void fill(const Event& event);

    /**
     * @brief Write one ROOT file and render the same histograms for the completed uniform run.
     * @param path Destination `<prefix>_monitoring_plots.root` file.
     * @param plot_directory Destination directory for rendered files.
     * @param pdf_name Multipage PDF filename inside plot_directory.
     * @throws std::runtime_error If ROOT cannot create or write an output.
     */
    void save(const std::filesystem::path& path, const std::filesystem::path& plot_directory, const std::string& pdf_name);

   private:
    /** @struct Impl @brief Owns ordered histogram definitions and particle/axis fill metadata. */
    struct Impl;
    std::unique_ptr<Impl> impl_;  ///< Exclusive monitoring state for one uniform run.
};

#pragma endregion

}  // namespace samples
