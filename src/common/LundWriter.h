//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.h
 * @brief LUND run-output interface and ownership.
 *
 * Purpose:
 *   Own file splitting and completion publication for both generation workflows.
 *
 * Workflow:
 *   Construct with a new output directory; write events until full; publish with finish after diagnostics.
 */

#pragma once
#include <fstream>
#include <vector>

#include "common/Event.h"
#include "common/RunConfig.h"
namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// A run is consumable only after finish() publishes manifest.json.
// LundWriter object ------------------------------------------------
#pragma region /* LundWriter object */
/**
 * @class LundWriter
 * @brief Own split LUND files and the completed-run manifest.
 *
 * Usage order: construct -> write until full -> save diagnostics externally -> finish.
 * The configuration must outlive the writer. A validated existing run directory is replaced,
 * preserving the legacy generator lifecycle.
 * Failure may leave partial output; only finish publishes manifest.json.
 */
class LundWriter {
   public:
    /** @brief Claim a new output directory using settings that outlive this instance. */
    LundWriter(const RunConfig& config, std::string workflow);
    /** @brief Return whether the configured event capacity has been reached. */
    bool full() const;
    /** @brief Write one nonempty event, rotating files as needed and updating counts. */
    void write(const Event& event);
    /** @brief Close output and publish provenance; scanned includes rejected input entries. */
    void finish(std::uint64_t scanned);
    /** @brief Return the number of events successfully serialized so far. */
    std::uint64_t count() const { return count_; }

    /** @brief Print a legacy-style workflow summary before and after generation. */
    static void printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned = 0, std::uint64_t written = 0, bool final = false);

    // Owned state --------------------------------------------------------------
   private:
    // Output object ------------------------------------------------
#pragma region /* Output object */
    /**
     * @struct Output
     * @brief Manifest bookkeeping for one LUND file owned by this run.
     *
     * Purpose: retain exact file counts, including a partially filled last file.
     * Lifecycle: write appends a record on rollover and increments events after serialization;
     * finish exports the records. The active stream belongs to LundWriter, not this record.
     */
    struct Output {
        std::string path;           ///< Run-relative lundfiles path, exported to the manifest.
        std::uint64_t events = 0;   ///< Successfully serialized events in this file.
    };
#pragma endregion
    // Borrowed configuration must outlive the writer; remaining members are owned run state.
    const RunConfig& config_;
    std::string workflow_;
    std::filesystem::path directory_;
    std::ofstream stream_;
    std::vector<Output> files_;  ///< Ordered manifest records; the last corresponds to stream_.
    std::uint64_t count_ = 0;
    std::uint64_t events_per_file_, capacity_;
    bool legacy_format_;
};
#pragma endregion
#pragma endregion
}  // namespace samples
