//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.h
 * @brief Declares how physical input selects its converter.
 *
 * Purpose:
 *   Let callers convert physical input without knowing its file layout. Each supported format has a
 *   small converter that produces the common Event values used by the shared LUND code.
 *
 * Workflow:
 *   The create-lund command checks RunConfig -> convertPhysical reads `event-generator` -> the matching
 *   converter reads the input and sends Event values to the shared vertex and output code -> that
 *   converter finishes the run.
 *
 * Inputs:
 *   RunConfig contains the selected format, input, target, beam, event limits, output, and recorded
 *   metadata. Each converter reads only the file layout for its own format.
 *
 * Outputs:
 *   This function creates no files itself. The selected converter creates the split LUND files and run log.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig and must keep it alive until convertPhysical returns. The function and the
 *   selected converter only read it and do not store it afterward.
 *
 * Adding a format:
 *   Adding another format requires one converter and one branch in convertPhysical(). It must reuse the
 *   shared target sampling, LUND writing, naming, splitting, and run-log code.
 *
 * Failure:
 *   An unsupported generator is rejected before conversion begins. Configuration, input, conversion,
 *   target, and output errors pass unchanged to the command-line program.
 */

#pragma once
#include "core/config/RunConfig.h"

namespace samples {

// Physical conversion entry point ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Physical conversion entry point */

/**
 * @brief Call the converter selected by the configured event-generator name.
 *
 * Purpose:
 *   Select one converter from `event-generator` while keeping format-specific ROOT types and reading
 *   rules out of callers and shared LUND code.
 *
 * Workflow:
 *   Read the final generator name, call its converter, wait for it to finish, and reject a name that has
 *   no matching branch.
 *
 * Inputs:
 *   config belongs to the caller and is read only. The current code accepts `genie-gst`, which is also
 *   the default when the user does not choose another format.
 *
 * Outputs:
 *   No C++ value is returned. A successful call leaves a completed physical LUND run created by the
 *   converter and shared writer.
 *
 * Rules:
 *   Exactly one converter is called. This function does not generate interactions, change generator
 *   truth, sample uniform kinematics, or submit detector-simulation jobs.
 *
 * Failure:
 *   Throws std::runtime_error when `event-generator` has no converter. Errors from the selected
 *   converter pass to the caller.
 *
 * @param config Checked physical-run settings read during this call and not stored afterward.
 *
 * @throws std::runtime_error If no converter implements the configured generator.
 */
void convertPhysical(const RunConfig& config);

#pragma endregion

}  // namespace samples
