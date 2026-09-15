//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file UniformGenerator.h
 * @brief Uniform generation entry-point contract.
 *
 * Purpose:
 *   Expose acceptance-sample generation to the CLI without exposing loop internals.
 *
 * Workflow:
 *   Pass resolved settings to generateUniform; consume the completed run through its manifest.
 */

#pragma once
#include "common/RunConfig.h"
namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

/**
 * @brief Generate the requested acceptance channel and its completed run outputs.
 * @param config Resolved and validated sample settings.
 * @note Writes LUND and diagnostics before publishing a manifest; failures throw.
 */
void generateUniform(const RunConfig& config);
#pragma endregion
}
