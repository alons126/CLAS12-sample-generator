//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.cpp
 * @brief Physical event-generator adapter dispatch.
 *
 * Purpose:
 *   Route the shared physical workflow to a small input-format adapter without
 *   duplicating LUND creation or exposing generator-specific names to users.
 *
 * Workflow:
 *   Read the validated event-generator setting, invoke the matching adapter, and
 *   reject unsupported values explicitly.
 */

#include "physical/PhysicalConverter.h"

#include <stdexcept>

#include "genie/GenieConverter.h"

namespace samples {

// convertPhysical -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* convertPhysical */

/**
 * @brief Dispatch one physical conversion run to its concrete adapter.
 *
 * @param config Validated physical-source configuration.
 *
 * @throws std::runtime_error If event-generator does not name an implemented adapter.
 */
void convertPhysical(const RunConfig& config) {
    if (config.get("event-generator") == "genie") {
        convertGenie(config);
        return;
    }

    throw std::runtime_error("Unsupported physical event generator: " + config.get("event-generator"));
}

#pragma endregion

}  // namespace samples
