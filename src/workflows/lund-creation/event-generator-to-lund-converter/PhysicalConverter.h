//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.h
 * @brief Declares how the physical LUND converter selects an input-file reader.
 *
 * Purpose:
 *   Different event-generator file formats store data differently. This interface selects the reader for
 *   the requested format, so the shared LUND code does not need format-specific checks.
 *
 * Workflow:
 *   The `create-lund` command checks RunConfig -> convertPhysical reads `event-generator` -> the matching
 *   reader copies supported input events into Event objects -> shared code writes the LUND files.
 *
 * Inputs:
 *   RunConfig contains the selected input format, input path, target and beam settings, event limits,
 *   output path, and labels recorded in the run log.
 *
 * Outputs:
 *   This function creates no files itself. The selected reader and shared writer create the split LUND
 *   files and run log.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig and keeps it alive until convertPhysical returns. This function and the
 *   selected reader do not store it.
 *
 * Adding an input format:
 *   Add one format reader and one selection branch in convertPhysical(). Reuse the existing code for
 *   target positions, LUND writing, filenames, file splitting, and the run log.
 *
 * Failure:
 *   An unknown `event-generator` value is rejected before input conversion begins. Other errors are
 *   passed to the command-line program for printing.
 */

#pragma once

#include "core/config/RunConfig.h"

namespace samples {

// Physical conversion entry point ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Physical conversion entry point */

/**
 * @brief Run the input reader named by the `event-generator` setting.
 *
 * Purpose:
 *   For example, `event-generator = genie-gst` selects the code that understands a GENIE GST ROOT tree.
 *   Callers can start physical LUND conversion without knowing that tree's fields or ROOT types.
 *
 * Workflow:
 *   Read the selected name, call its matching reader, and return when that reader finishes. Reject a name
 *   that has no matching reader.
 *
 * Inputs:
 *   config belongs to the caller and is read only. The current code accepts `genie-gst`, which is also
 *   the default.
 *
 * Outputs:
 *   No C++ value is returned. A successful call leaves the LUND files and run log created by the selected
 *   reader and shared writer.
 *
 * Rules:
 *   Exactly one reader is called. The physical LUND converter copies supported particles from existing
 *   input. It does not run an event generator, change particle momenta, or start detector simulation.
 *
 * Failure:
 *   Throws std::runtime_error when `event-generator` has no matching reader. Errors from that reader pass
 *   to the caller.
 *
 * @param config Checked physical-run settings read during this call and not stored afterward.
 * @throws std::runtime_error If no reader supports the configured input format.
 */
void convertPhysical(const RunConfig& config);

#pragma endregion

}  // namespace samples
