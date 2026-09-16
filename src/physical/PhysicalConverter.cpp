/**
 * @file PhysicalConverter.cpp
 * @brief Physical event-generator adapter dispatch.
 *
 * Workflow: validate generator name -> call its concrete input adapter.
 */
#include "physical/PhysicalConverter.h"
#include <stdexcept>
#include "genie/GenieConverter.h"
namespace samples {
#pragma region /* convertPhysical */
void convertPhysical(const RunConfig& config) {
    if (config.get("event-generator") == "genie") { convertGenie(config); return; }
    throw std::runtime_error("Unsupported physical event generator: " + config.get("event-generator"));
}
#pragma endregion
}  // namespace samples
