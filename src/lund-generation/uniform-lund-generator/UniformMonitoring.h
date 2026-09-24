//
// Created by Alon Sportes on 19/09/2026.
//

/**
 * @file UniformMonitoring.h
 * @brief Declares the plots created for uniform samples.
 *
 * Purpose:
 *   Store all uniform monitoring histograms in one ROOT file and render them with the established layout
 *   and style. Physical conversion does not use this class.
 *
 * Workflow:
 *   Construct from the resolved uniform sample identity -> fill after each written event -> save one
 *   ROOT file -> render the same ordered histograms as a PDF and numbered PNG files.
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "core/lund/Event.h"

namespace samples {

// UniformMonitoring object ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformMonitoring object */

/**
 * @class UniformMonitoring
 * @brief Own all monitoring histograms for one uniform channel.
 *
 * Purpose:
 *   Create the correct plots for 1e, electron-tester, and every supported electron-hadron sample.
 *   Electron-hadron ROOT names and titles include FD or CD.
 *
 * Ownership and lifetime:
 *   The private implementation owns the ROOT histograms for one run. The object cannot be copied and
 *   must stay alive from before the event loop until save() finishes.
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
     * @throws std::runtime_error If an electron-hadron label is paired with an unsupported hadron PDG
     *         code. Allocation failures also propagate.
     */
    UniformMonitoring(std::string sample_label, int hadron_pid, double beam);

    /** @brief Release the private implementation and its detached ROOT histograms. */
    ~UniformMonitoring();

    /**
     * @brief Fill every configured histogram from one successfully written event.
     * @param event Uniform event read during this call and not stored or changed.
     * @throws std::runtime_error If the event lacks a required electron or hadron.
     */
    void fill(const Event& event);

    /**
     * @brief Write one ROOT file and render the same histograms for the completed uniform run.
     * @param path Destination `<prefix>_monitoring_plots.root` file.
     * @param plot_directory Destination directory for rendered files.
     * @param pdf_name Multipage PDF filename inside plot_directory.
     * @throws std::runtime_error If ROOT cannot create or write an output. Filesystem errors while
     *         creating the plot directory also propagate.
     */
    void save(const std::filesystem::path& path, const std::filesystem::path& plot_directory, const std::string& pdf_name);

   private:
    /** @struct Impl @brief Stores the ordered histograms and the values used to fill their axes. */
    struct Impl;
    std::unique_ptr<Impl> impl_;  ///< Histograms and fill settings for one uniform run.
};

#pragma endregion

}  // namespace samples
