//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.h
 * @brief Declares the generator-independent physical conversion boundary.
 *
 * Purpose:
 *   Keep workflow code independent of concrete event-generator input formats.
 *   New physical adapters connect through this dispatch boundary while shared
 *   LUND output remains in the common layer.
 *
 * Workflow:
 *   The create-lund workflow resolves RunConfig, then calls convertPhysical(),
 *   which selects the adapter named by `event-generator`.
 */

#pragma once
#include "config/RunConfig.h"

namespace samples {

// Physical conversion entry point ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Physical conversion entry point */

/**
 * @brief Dispatch configured physical input to its event-generator adapter.
 *
 * @param config Validated physical-source run configuration borrowed for the conversion.
 *
 * @throws std::runtime_error If no adapter implements the configured generator.
 */
void convertPhysical(const RunConfig& config);

#pragma endregion

}  // namespace samples
