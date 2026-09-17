//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverter.h
 * @brief GENIE GST conversion entry-point contract.
 *
 * Purpose:
 *   Expose conversion of existing physical events to the CLI.
 *
 * Workflow:
 *   Pass input and output settings to convertGenie; consume the completed LUND run through its manifest.
 */

#pragma once
#include "config/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

/**
 * @brief Convert supported events from an existing GENIE GST input into LUND.
 * @param config Resolved input, output, target and metadata settings.
 * @note Reads physical events rather than generating interactions; failures throw.
 */
void convertGenie(const RunConfig& config);
#pragma endregion

}  // namespace samples
