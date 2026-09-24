//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.cpp
 * @brief Chooses the converter for one physical input format.
 *
 * Purpose:
 *   Map the configured event-generator name to its converter. This file does not read generator data,
 *   build events, sample vertices, or write output.
 *
 * Workflow:
 *   Read `event-generator` -> call exactly one matching converter -> return after it finishes, or reject
 *   an unsupported name.
 *
 * Inputs:
 *   RunConfig contains the final generator name and all settings needed by its converter. This file reads
 *   only the generator name and passes the complete object unchanged.
 *
 * Outputs:
 *   This function returns no value and creates no files itself. The selected converter and LundWriter
 *   create and report the output.
 *
 * Design:
 *   Each supported format has one visible branch. Adding a format requires its converter and one new
 *   branch here; the shared LUND workflow does not need to be copied.
 *
 * Failure:
 *   An unsupported name throws before conversion starts. Errors from the selected converter pass to the
 *   command-line program, which prints them and returns a nonzero status.
 */

#include "event-generator-to-lund-converter/PhysicalConverter.h"

#include <stdexcept>

#include "event-generator-to-lund-converter/genie-gst/GenieConverterGST.h"

namespace samples {

// convertPhysical -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertPhysical */

void convertPhysical(const RunConfig& config) {
    // Use an exact name match and pass the same unchanged settings to the GENIE converter.
    if (config.get("event-generator") == "genie-gst") {
        convertGenieGST(config);

        // Stop after the selected converter completes.
        return;
    }

    // Fail before any converter can prepare or replace an output directory.
    throw std::runtime_error("Unsupported physical event generator: " + config.get("event-generator"));
}

#pragma endregion

}  // namespace samples
