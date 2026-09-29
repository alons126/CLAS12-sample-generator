//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.cpp
 * @brief Selects the input reader used by the physical LUND converter.
 *
 * Purpose:
 *   Map the configured `event-generator` name to code that understands that input format. This file does
 *   not read event data, choose vertex positions, or write output.
 *
 * Call path:
 *   Read `event-generator` -> call exactly one matching reader -> return after it finishes, or reject an
 *   unsupported name.
 *
 * Inputs:
 *   RunConfig contains the input-format name and all settings needed by its reader. This file reads only
 *   that name and passes the complete object unchanged.
 *
 * Outputs:
 *   This function returns no value and creates no files itself. The selected reader and LundWriter create
 *   and report the output.
 *
 * Adding an input format:
 *   Give the new format its own reader and add one branch here. Do not copy the shared LUND-writing code.
 *
 * Failure:
 *   An unsupported name throws before input conversion starts. Errors from the selected reader pass to
 *   the command-line program, which prints them and returns a nonzero status.
 */

#include "event-generator-to-lund-converter/PhysicalConverter.h"

#include <stdexcept>

#include "event-generator-to-lund-converter/genie-gst/GenieConverterGST.h"

namespace samples {

// convertPhysical -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertPhysical */

void convertPhysical(const RunConfig& config) {
    // `genie-gst` means that the input is a GENIE GST ROOT tree. Pass every setting unchanged to its reader.
    if (config.getText("event-generator") == "genie-gst") {
        convertGenieGST(config);

        // The selected reader performed the complete physical LUND conversion, so there is nothing else to run.
        return;
    }

    // Fail before any format reader can prepare or replace an output directory.
    throw std::runtime_error("Unsupported physical event generator: " + config.getText("event-generator"));
}

#pragma endregion

}  // namespace samples
