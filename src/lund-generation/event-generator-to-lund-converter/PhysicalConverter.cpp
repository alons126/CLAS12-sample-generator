//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.cpp
 * @brief Physical event-generator adapter dispatch.
 *
 * Purpose:
 *   Implement the single generator-independent routing point for physical LUND conversion. The
 *   dispatcher knows which adapter corresponds to each public event-generator identifier but does not
 *   know generator schemas, construct events, sample vertices, or serialize output itself.
 *
 * Workflow:
 *   Receive resolved physical configuration -> inspect `event-generator` -> invoke exactly one matching
 *   synchronous adapter -> return after that adapter finalizes the run, or reject an unsupported value.
 *
 * Inputs:
 *   RunConfig supplies the already resolved generator identifier and every setting borrowed by the
 *   selected adapter. This layer reads only `event-generator` and passes the complete object unchanged.
 *
 * Outputs:
 *   The dispatcher returns no value and creates no files directly. Output artifacts and terminal
 *   reporting belong to the selected adapter and the shared LUND writer it uses.
 *
 * Design:
 *   Explicit branches keep the supported set visible and the call chain direct. Adding a generator
 *   requires including its adapter and adding one branch here; it does not require a registry, plugin
 *   lifecycle, generic orchestration layer, or duplicated output workflow.
 *
 * Failure:
 *   Unsupported identifiers throw before any adapter is invoked. Exceptions from a selected adapter are
 *   intentionally not caught here and propagate to the command-line boundary for one consistent failure
 *   report and nonzero exit status.
 */

#include "event-generator-to-lund-converter/PhysicalConverter.h"

#include <stdexcept>

#include "event-generator-to-lund-converter/genie-gst/GenieConverter.h"

namespace samples {

// convertPhysical -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertPhysical */

/**
 * @brief Dispatch one physical conversion run to its concrete adapter.
 *
 * Purpose:
 *   Translate the resolved public generator name into one concrete adapter call while preserving a
 *   generator-neutral interface for the application entry point.
 *
 * Workflow:
 *   Compare `event-generator` with each implemented identifier, call the first exact match, return after
 *   successful completion, and throw when no branch matches.
 *
 * Inputs:
 *   config is a caller-owned physical-source RunConfig. It is borrowed as const, forwarded unchanged to
 *   the selected adapter and never retained beyond this synchronous call.
 *
 * Outputs:
 *   No C++ value or dispatcher-owned artifact is produced. A successful return means the selected adapter
 *   completed its own conversion and output-publication contract.
 *
 * Failure:
 *   Throws std::runtime_error for an unsupported generator. Any validation, input, target or output
 *   exception from the selected adapter propagates unchanged; this function does not mask partial runs.
 *
 * @param config Resolved and validated physical-source configuration borrowed for this call.
 *
 * @throws std::runtime_error If event-generator does not name an implemented adapter.
 */
void convertPhysical(const RunConfig& config) {
    // Keep dispatch as a direct exact-name comparison. The adapter receives the same immutable RunConfig
    // used by the surrounding workflow and owns all GENIE-GST-specific validation, reading and conversion.
    if (config.get("event-generator") == "genie-gst") {
        convertGenie(config);

        // A completed adapter call satisfies this dispatch request; return explicitly so later adapter
        // branches or the unsupported-generator failure can never run for the same conversion.
        return;
    }

    // No adapter has claimed the configured identifier. Fail before generator-specific code can prepare
    // or replace an output directory, and include the exact rejected value in the operator-facing error.
    throw std::runtime_error("Unsupported physical event generator: " + config.get("event-generator"));
}

#pragma endregion

}  // namespace samples
