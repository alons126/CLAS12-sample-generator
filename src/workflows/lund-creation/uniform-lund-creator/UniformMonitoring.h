/**
 * @file UniformMonitoring.h
 * @brief Defines the monitoring plots used to check uniform LUND events.
 *
 * Purpose:
 *   Monitoring plots let the user check that the uniform LUND creator produced the expected momentum,
 *   angles, and vertex positions. UniformMonitoring collects those values in ROOT histograms, saves all
 *   histograms in one ROOT file, and draws them as a PDF and separate PNG images. The physical LUND
 *   converter does not create these plots.
 *
 * Execution flow:
 *   Create one object for the selected sample. After each event is written successfully, call
 *   addEventToHistograms() with that event. After the event loop ends, call
 *   saveHistogramsAndRenderPlots(). It writes the histograms to one ROOT file and draws the same plots, in
 *   the same order, into one multipage PDF and numbered PNG files.
 *
 * Plot ranges:
 *   Every z-position plot covers -8 through 5 centimeters so it can show events from every supported RG-M
 *   target. Momentum plots extend slightly above the beam energy. Angular and other position ranges depend
 *   on the selected channel and detector region.
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "core/lund/Event.h"

namespace samples {

// Uniform monitoring plots ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Uniform monitoring plots */

/**
 * @class UniformMonitoring
 * @brief Owns and fills all monitoring histograms for one uniform sample.
 *
 * Purpose:
 *   Create only the plots needed by the selected channel. A one-electron sample, the electron test, and an
 *   electron-hadron sample each use a different plot set. Electron-hadron plot names and titles include
 *   FD for the forward detector or CD for the central detector.
 *
 * Ownership and lifetime:
 *   The hidden Impl object owns every ROOT histogram. UniformMonitoring cannot be copied. Create it before
 *   the event loop and keep it alive until saveHistogramsAndRenderPlots() has finished.
 *
 * Rules:
 *   sample_label is a final uniform sample label. Electron-hadron events must contain the configured
 *   hadron after the electron. Plot momentum is measured in GeV/c, angles in degrees, and vertex positions
 *   in centimeters.
 */
class UniformMonitoring {
   public:
    /**
     * @brief Constructor: Create the complete ordered plot set for one uniform sample.
     * @param sample_label Final label such as `1e`, `electron-tester`, `epFD`, or `epipCD`.
     * @param hadron_pid Standard PDG integer for the configured hadron. Electron-only channels ignore it.
     * @param beam Beam energy in GeV. Momentum axes extend to 110% of this value.
     * @throws std::runtime_error If an electron-hadron sample uses an unsupported hadron.
     * @throws std::bad_alloc If memory for a histogram cannot be allocated.
     */
    UniformMonitoring(std::string sample_label, int hadron_pid, double beam);

    /** @brief Destructor: Release the hidden implementation and every ROOT histogram it owns. */
    ~UniformMonitoring();

    /**
     * @brief Add one successfully written event to every plot that uses its values.
     * @param event Uniform event to read. The object does not store or change it.
     * @throws std::runtime_error If the event does not contain a required electron or hadron.
     */
    void addEventToHistograms(const Event& event);

    /**
     * @brief Save the histograms and draw them for the completed uniform run.
     * @param path ROOT output file named `<prefix>__monitoring_plots.root`.
     * @param plot_directory Directory that receives the PDF and PNG files.
     * @param pdf_name Name of the multipage PDF inside plot_directory.
     * @throws std::runtime_error If ROOT cannot create the file or write a histogram.
     * @throws std::filesystem::filesystem_error If the plot directory cannot be created.
     */
    void saveHistogramsAndRenderPlots(const std::filesystem::path& path, const std::filesystem::path& plot_directory, const std::string& pdf_name);

    // Hidden ROOT state -------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /**
     * @struct Impl
     * @brief Stores the histograms and their axis instructions without exposing ROOT details in this header.
     */
    struct Impl;
    std::unique_ptr<Impl> impl_;  ///< Owns the histograms and their fill instructions for this run.
};

#pragma endregion

}  // namespace samples
