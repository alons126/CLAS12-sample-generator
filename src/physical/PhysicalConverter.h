/** @file PhysicalConverter.h @brief Generator-independent physical LUND conversion entry point. */
#pragma once
#include "common/RunConfig.h"
namespace samples {
/** @brief Dispatch physical input to the configured event-generator adapter. */
void convertPhysical(const RunConfig& config);
}
