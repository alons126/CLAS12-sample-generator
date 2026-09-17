//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file GenieConverter.h
 * @brief GENIE GST conversion entry-point contract.
 *
 * Purpose:
 *   Implement the GENIE-specific input adapter nested under the generator-independent
 *   clas12-generator-to-lund workflow.
 *
 * Workflow:
 *   Receive validated physical settings from convertPhysical, read existing GST truth,
 *   and publish the completed LUND run through its manifest.
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
