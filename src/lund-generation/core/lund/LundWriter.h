//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.h
 * @brief Writes split LUND files and the completed-run manifest.
 *
 * Purpose:
 *   Give uniform generation and physical conversion the same file format, splitting, naming, settings
 *   record, and completion marker. The writer does not create or select event content.
 *
 * Workflow:
 *   Construct from a checked RunConfig -> safely replace the exact run directory -> create its folders
 *   -> write nonempty Event records into split LUND files -> let the caller save monitoring -> finish()
 *   closes the files and renames the temporary manifest to lund-gen-log.json.
 *
 * Data contract:
 *   Particle momentum is in GeV/c, mass is in GeV/c², derived energy and beam energy are in GeV, and
 *   vertices are in cm. The event source supplies header values and particle order. The writer keeps
 *   them in that order and applies the project's fixed text format.
 */

#pragma once
#include <fstream>
#include <vector>

#include "core/config/RunConfig.h"
#include "core/lund/Event.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// LundWriter object -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter object */
/**
 * @class LundWriter
 * @brief Own split LUND files and the completed-run manifest.
 *
 * Purpose:
 *   Prepare the run directory safely, enforce the event limit, write split files, count written events,
 *   and publish the final settings and file list for both LUND sources.
 *
 * Use:
 *   1. Construct with a validated configuration and `uniform` or `physical` workflow label.
 *   2. The constructor rejects unsafe paths, reports and recursively replaces an existing exact run
 *      directory, and creates its `lundfiles/` child.
 *   3. Call write() in source-defined event order until full() becomes true or physical input ends.
 *   4. Save required monitoring outside this class.
 *   5. Call finish() once to close LUND output, write build/settings data and file counts to a temporary
 *      log, and rename it to `lund-gen-log.json` as the run-completion marker.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig and must keep it alive longer than the writer. This object owns its label,
 *   output path, stream, file list, counters, and limits. write() only reads each Event during the call.
 *
 * Invariants:
 *   count_ is the number of events written successfully. The sum of all per-file counts equals count_.
 *   files_ stays in creation order, and no event is accepted after count_ reaches capacity_.
 *
 * Failure behavior:
 *   Construction or I/O failures throw. A failed run may intentionally leave its partial directory and
 *   LUND files for inspection, but the run is incomplete because only finish() creates lund-gen-log.json.
 */
class LundWriter {
   public:
    /**
     * @brief Safely replace and initialize the exact configured run directory.
     * @param config Checked settings owned by the caller. They must outlive this writer.
     * @param workflow Manifest/summary label, expected to be `uniform` or `physical`; copied into the
     *                 writer without changing source-specific event semantics.
     * @throws std::exception If the path is unsafe, replacement/creation fails, or required limits and
     *         settings cannot be read.
     */
    LundWriter(const RunConfig& config, std::string workflow);

    /**
     * @brief Test whether the requested total written-event capacity has been reached.
     * @return True when count() is greater than or equal to capacity_ and write() must not be called.
     */
    bool full() const;

    /**
     * @brief Write one nonempty event and then update the file and run counts.
     * @param event Event to read. Its particle order, header values, and vertices are kept unchanged.
     *              The writer calculates particle energy from momentum and mass.
     * @throws std::exception If capacity is exhausted, the event is empty or non-finite, or file output
     *         fails. Counters advance only after the complete event is written.
     * @note Opens the first file lazily and rotates after RunConfig::events-per-file events. Uniform
     *       records display per-file event IDs; count() remains run-global.
     */
    void write(const Event& event);

    /**
     * @brief Close LUND output and publish the completed-run manifest with one final rename.
     * @param scanned Number of source events examined. It equals count() for uniform generation and may
     *                exceed count() when physical conversion rejects unsupported input interactions.
     * @throws std::exception If manifest writing, stream closure, or final rename fails.
     * @note Call only after required monitoring is saved; lund-gen-log.json marks the run as complete.
     */
    void finish(std::uint64_t scanned);

    /**
     * @brief Return the total number of events written so far.
     * @return Written-event count; this is also the next uniform event ID.
     */
    std::uint64_t count() const { return count_; }

    /**
     * @brief Print the setup or completion values, grouped by topic.
     * @param config Final settings used to choose displayed fields and paths.
     * @param workflow `uniform` or `physical`, selecting only values used by that source.
     * @param scanned Source entries examined; ignored by setup and printed by completion.
     * @param written Events written successfully; ignored by setup and printed by completion.
     * @param final Print only completion counters when true or only resolved setup fields when false.
     * @note Presentation only: this function creates no output files and does not determine run status.
     *       Fixed output constants and settings unused by the selected channel are omitted.
     */
    static void printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned = 0, std::uint64_t written = 0, bool final = false);

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Output object -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Output object */
    /**
     * @struct Output
     * @brief Path and event count for one LUND file in the manifest.
     *
     * Purpose:
     *   Store the path relative to the run directory and the number of events written to that file.
     *
     * Use and ownership:
     *   write() adds a record when it opens a file and increases events after each successful write.
     *   finish() writes the records in order. Output owns its path and count; LundWriter owns the stream.
     *
     * Invariants:
     *   path is relative to the run directory and lies under `lundfiles/`; events never exceeds the
     *   writer's per-file limit; and only the final record may be partially filled during a full run.
     */
    struct Output {
        std::string path;          ///< Run-relative lundfiles path, exported to the manifest.
        std::uint64_t events = 0;  ///< Events written successfully to this file.
    };
#pragma endregion

    // Settings owned by the caller --------------------------------------------------------------------------------------------------------------------------------------
    const RunConfig& config_;  ///< Final settings owned by the caller and written to the manifest.

    // Owned run identity and filesystem state ---------------------------------------------------------------------------------------------------------------------------
    std::string workflow_;             ///< Manifest/summary source label: uniform or physical.
    std::filesystem::path directory_;  ///< Absolute final run directory with `.` and `..` removed.
    std::ofstream stream_;             ///< Active LUND stream; opened lazily and closed by finish().
    std::vector<Output> files_;        ///< Ordered manifest records; the last corresponds to stream_.

    // Owned counters and file-splitting rules ---------------------------------------------------------------------------------------------------------------------------
    std::uint64_t count_ = 0;        ///< Total number of events written successfully.
    std::uint64_t events_per_file_;  ///< Positive source-specific split threshold from RunConfig.
    std::uint64_t capacity_;         ///< Maximum written events requested by RunConfig::events.
};
#pragma endregion

#pragma endregion

}  // namespace samples
