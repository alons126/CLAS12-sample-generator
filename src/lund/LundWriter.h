//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.h
 * @brief LUND run-output interface and ownership.
 *
 * Purpose:
 *   Give uniform generation and physical conversion one serialization, file-splitting, naming,
 *   provenance, and completed-run contract while keeping source-specific event production outside the
 *   writer.
 *
 * Workflow:
 *   Construct from a validated RunConfig -> guard and replace the exact resolved run directory -> create
 *   the common layout plus archived uniform directories when applicable -> write nonempty Event records
 *   into configured-size LUND files -> let the workflow save diagnostics -> close output and atomically
 *   publish manifest.json through finish().
 *
 * Data contract:
 *   Particle momentum is in GeV/c, mass is in GeV/c², derived energy and beam energy are in GeV, and
 *   vertices are in cm. Header metadata and particle ordering are supplied by the event source; this
 *   component preserves them while applying the selected legacy or precise text format.
 */

#pragma once
#include <fstream>
#include <vector>

#include "config/RunConfig.h"
#include "lund/Event.h"

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
 *   Centralize destructive run-directory preparation, capacity enforcement, LUND serialization,
 *   rollover bookkeeping, and final provenance for both supported LUND source workflows.
 *
 * Lifecycle:
 *   1. Construct with a validated configuration and `uniform` or `physical` workflow label.
 *   2. The constructor rejects unsafe paths, reports and recursively replaces an existing exact run
 *      directory, and creates its `lundfiles/` child.
 *   3. Call write() in source-defined event order until full() becomes true or physical input ends.
 *   4. Save required monitoring outside this class.
 *   5. Call finish() once to close LUND output, write provenance and file counts to a temporary
 *      manifest, and rename it to `manifest.json` as the run-completion marker.
 *
 * Ownership and lifetime:
 *   The RunConfig is borrowed by const reference and must outlive the writer. The workflow label,
 *   normalized directory, stream, per-file records, counters, limits, and format selection are owned
 *   by this object. Event arguments are borrowed only for the duration of write().
 *
 * Invariants:
 *   count_ equals the number of successfully serialized events; the sum of Output::events equals
 *   count_; files_ follows creation order; and its last record describes the active/latest stream.
 *   No event is accepted after count_ reaches capacity_.
 *
 * Failure behavior:
 *   Construction or I/O failures throw. A failed run may intentionally leave its partial directory and
 *   LUND files for inspection, but it is not consumable because only finish() publishes manifest.json.
 */
class LundWriter {
   public:
    /**
     * @brief Claim and initialize the exact configured run directory.
     * @param config Borrowed validated settings; this object retains a const reference, so config must
     *               outlive the writer.
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
     * @brief Serialize one nonempty event and update file/run counts after successful output.
     * @param event Borrowed event whose particle order, header metadata, and vertex values are
     *              preserved. The writer derives particle energy from momentum and mass.
     * @throws std::exception If capacity is exhausted, the event is empty or non-finite, or file output
     *         fails. Counters advance only after serialization succeeds.
     * @note Opens the first file lazily and rotates after RunConfig::events-per-file events. Uniform
     *       legacy formatting may display per-file event IDs; count() remains run-global.
     */
    void write(const Event& event);

    /**
     * @brief Close LUND output and publish the completed-run manifest atomically.
     * @param scanned Number of source events examined. It equals count() for uniform generation and may
     *                exceed count() when physical conversion rejects unsupported input interactions.
     * @throws std::exception If manifest writing, stream closure, or final rename fails.
     * @note Call only after required diagnostics are saved; manifest.json declares the run consumable.
     */
    void finish(std::uint64_t scanned);

    /**
     * @brief Return the run-global number of events successfully serialized so far.
     * @return Written-event count; this is also the next uniform event ID.
     */
    std::uint64_t count() const { return count_; }

    /**
     * @brief Print the source-appropriate legacy-style run summary.
     * @param config Borrowed resolved settings used for displayed metadata.
     * @param workflow `uniform` or `physical`, selecting the matching summary fields.
     * @param scanned Physical input entries examined; ignored by the initial summary.
     * @param written Successfully serialized output events; used by the final summary.
     * @param final Print completion counters when true or configuration/input details when false.
     * @note Presentation only: this function creates no output files and does not determine run status.
     */
    static void printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned = 0, std::uint64_t written = 0, bool final = false);

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Output object -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Output object */
    /**
     * @struct Output
     * @brief Manifest bookkeeping for one LUND file owned by this run.
     *
     * Purpose:
     *   Retain the run-relative location and exact successful event count for every split file,
     *   including a partially filled final physical-conversion file.
     *
     * Lifecycle and ownership:
     *   write() appends one record when it lazily opens or rotates a file, then increments events only
     *   after successful serialization. finish() exports records in order. Output owns its string and
     *   counter; the active std::ofstream remains owned by LundWriter.
     *
     * Invariants:
     *   path is relative to the run directory and lies under `lundfiles/`; events never exceeds the
     *   writer's per-file limit; and only the final record may be partially filled during a full run.
     */
    struct Output {
        std::string path;          ///< Run-relative lundfiles path, exported to the manifest.
        std::uint64_t events = 0;  ///< Successfully serialized events in this file.
    };
#pragma endregion

    // Borrowed configuration --------------------------------------------------------------------------------------------------------------------------------------------
    const RunConfig& config_;  ///< Immutable source of resolved settings and manifest provenance.

    // Owned run identity and filesystem state ---------------------------------------------------------------------------------------------------------------------------
    std::string workflow_;             ///< Manifest/summary source label: uniform or physical.
    std::filesystem::path directory_;  ///< Absolute normalized final run directory.
    std::ofstream stream_;             ///< Active LUND stream; opened lazily and closed by finish().
    std::vector<Output> files_;        ///< Ordered manifest records; the last corresponds to stream_.

    // Owned counters and serialization policy ---------------------------------------------------------------------------------------------------------------------------
    std::uint64_t count_ = 0;        ///< Successfully serialized run-global event count.
    std::uint64_t events_per_file_;  ///< Positive source-specific split threshold from RunConfig.
    std::uint64_t capacity_;         ///< Maximum written events requested by RunConfig::events.
    bool legacy_format_;             ///< Select archived precision/spacing/ID behavior when true.
};
#pragma endregion

#pragma endregion

}  // namespace samples
