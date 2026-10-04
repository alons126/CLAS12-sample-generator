/**
 * @file LundWriter.h
 * @brief Writes events to numbered LUND files and records the completed run.
 *
 * Purpose:
 *   The uniform LUND creator makes random test events. The physical LUND converter reads existing
 *   event-generator events. Both pass completed Event objects to LundWriter so their output uses the same
 *   LUND text format, filename rules, file splitting, internal output folders, and completion manifest.
 *   RunConfig chooses the final run-directory path. LundWriter does not choose particles or generate their
 *   motion; it prepares that directory and writes the completed events and run manifest. GEMC defines the
 *   LUND event-header and particle-record fields written here. For more information, see:
 *   https://gemc.jlab.org/gemc/html/documentation/generator/lund.html
 *
 * Execution flow:
 *   Create the writer from a checked RunConfig. The constructor checks the exact run-directory path,
 *   replaces that directory if it already exists, and creates the required folders. Call writeEvent() for
 *   each event in order. After the uniform LUND creator saves its monitoring output, call finalizeRun().
 *   finalizeRun() closes the LUND file and publishes `lund-creation-log.json`, which marks the run as complete.
 *
 * Written data:
 *   Particle momentum is measured in GeV/c, mass in GeV/c², energy and beam energy in GeV, and vertex
 *   positions in centimeters. The uniform LUND creator or physical LUND converter supplies the event
 *   header values and particle order. The writer keeps that order, calculates each particle's energy from
 *   its mass and momentum, and writes the same fixed LUND text format for every channel. Header fields
 *   4, 5, and 7 have six decimal places; field 10 has two.
 */

#pragma once

#include <fstream>
#include <vector>

#include "core/config/RunConfig.h"
#include "core/lund/Event.h"
#include "support/environment.h"

namespace samples {

// LUND output interface -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LUND output interface */

// LundWriter class ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter class */
/**
 * @class LundWriter
 * @brief Owns the numbered LUND files and completed-run manifest for one run.
 *
 * Purpose:
 *   Prepare the run directory safely, stop writes at the requested event limit, open a new numbered file
 *   when the current file reaches its limit, count successful writes, and record the settings and output
 *   files when the run finishes.
 *
 * Use:
 *   Create one writer with checked settings. Call writeEvent() for each event in order. Save any required
 *   monitoring output outside this class. Call finalizeRun() once to publish `lund-creation-log.json`.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig and must keep it alive until the writer is destroyed. LundWriter owns its
 *   workflow name, output path, open file, list of created files, counters, and limits. writeEvent() reads
 *   an Event only during that call and does not keep a reference to it.
 *
 * Rules:
 *   count_ equals the number of completely written events. Adding the event counts from every file gives
 *   the same number. files_ remains in file-creation order. writeEvent() rejects another event after count_
 *   reaches capacity_.
 *
 * Failure:
 *   An unsafe directory path or a file-writing problem reports an error. A failed run may leave its
 *   partial directory and LUND files for inspection. It is still clearly incomplete because only
 *   finalizeRun() creates `lund-creation-log.json`.
 */
class LundWriter {
   public:
    /**
     * @brief Constructor: Check, replace, and prepare the exact configured run directory.
     * @param config Checked settings owned by the caller. They must outlive this writer.
     * @param workflow Exact manifest value `uniform` for the uniform LUND creator or `physical` for the
     *                 physical LUND converter. Both create empty `mchipo` and `reconhipo` directories.
     *                 Only `uniform` creates the directory for rendered monitoring plots.
     * @throws std::exception If the path could delete an unsafe location, a directory cannot be replaced
     *                        or created, or a required setting cannot be read.
     */
    LundWriter(const RunConfig& config, std::string workflow);

    /**
     * @brief Check whether the requested number of events has already been written.
     * @return true when getWrittenEventCount() has reached the configured event limit; otherwise false.
     */
    bool hasReachedRunEventLimit() const { return count_ >= capacity_; }

    /**
     * @brief Write one event and increase the counts after the complete event is written.
     * @param event Event to write. Particle order, header values, and vertex positions stay unchanged. The
     *              writer calculates each particle's energy from its momentum and mass.
     * @throws std::exception If the run has reached its event limit, the event contains no particles or
     *                        invalid numeric values, its ID does not fit the LUND header integer field, or
     *                        file output fails.
     * @note The first call opens file 1. A new file opens after the configured events-per-file limit. The
     *       event ID is written unchanged, so starting a new file does not restart event numbering.
     */
    void writeEvent(const Event& event);

    /**
     * @brief Close the LUND file and publish the manifest that marks the run complete.
     * @param scannedEventCount Number of possible source events examined. For the uniform LUND creator,
     *                          this equals getWrittenEventCount(). For the physical LUND converter, it may
     *                          be larger because some input events are not supported and are skipped.
     * @throws std::exception If the LUND file cannot close, the manifest cannot be written, or its final
     *                        rename fails.
     * @note Call this only after required monitoring output has been saved.
     */
    void finalizeRun(std::uint64_t scannedEventCount);

    /**
     * @brief Get the number of events written completely so far.
     * @return Written-event count. The uniform LUND creator also uses this as the next event ID.
     */
    std::uint64_t getWrittenEventCount() const { return count_; }

    /**
     * @brief Print either the checked setup or the final event counts.
     * @param config Final settings used to choose displayed fields and paths.
     * @param workflow `uniform` for the uniform LUND creator or `physical` for the physical LUND
     *                 converter. Only settings used by that workflow are shown.
     * @param scanned Possible source events examined. Used only in the final report.
     * @param written Events written completely. Used only in the final report.
     * @param final true to print final counts; false to print the setup.
     * @note This function only prints information. It does not create files or decide whether the run
     *       succeeded. It omits settings unused by the selected channel. Normal values are aligned against
     *       the right side of the summary; paths remain left-aligned and are not surrounded by quotes.
     */
    static void printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned = 0, std::uint64_t written = 0, bool final = false);

    // Stored writer state -----------------------------------------------------------------------------------------------------------------------------------------------
   private:
    // One output-file record --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* One output-file record */
    /**
     * @struct Output
     * @brief Stores the path and event count for one created LUND file.
     *
     * Purpose:
     *   Keep the information needed to list this file in the completed-run manifest.
     *
     * Use and ownership:
     *   writeEvent() adds a record when it opens a file and increases events after each complete event.
     *   finalizeRun() writes the records in the same order. Output owns its path and count. LundWriter
     *   separately owns the open file.
     *
     * Rules:
     *   path starts below the run directory and points inside `lundfiles/`. events cannot exceed the
     *   per-file limit. In a run that reaches its requested total, only the last file may contain fewer
     *   events than that limit.
     */
    struct Output {
        std::string path;          ///< Path below the run directory, beginning with `lundfiles/`.
        std::uint64_t events = 0;  ///< Number of complete events written to this file.
    };
#pragma endregion

    // Settings owned by the caller --------------------------------------------------------------------------------------------------------------------------------------
    const RunConfig& config_;  ///< Final settings written to the manifest; the caller owns this object.

    // Run name and files ------------------------------------------------------------------------------------------------------------------------------------------------
    std::string workflow_;             ///< Manifest and summary value: `uniform` or `physical`.
    std::filesystem::path directory_;  ///< Absolute run directory after removing `.` and `..` parts.
    std::ofstream stream_;             ///< Currently open LUND file; writeEvent() opens it and finalizeRun() closes it.
    std::vector<Output> files_;        ///< Created files in order; the last record matches stream_.

    // Event counts and limits -------------------------------------------------------------------------------------------------------------------------------------------
    std::uint64_t count_ = 0;        ///< Total number of completely written events.
    std::uint64_t events_per_file_;  ///< Number of events allowed in each LUND file.
    std::uint64_t capacity_;         ///< Maximum number of events allowed in the complete run.
};
#pragma endregion

#pragma endregion

}  // namespace samples
