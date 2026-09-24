//
// Created by Alon Sportes on 16/09/2026.
//

/**
 * @file PhysicalConverter.h
 * @brief Declares the generator-independent physical conversion boundary.
 *
 * Purpose:
 *   Keep the user-facing physical workflow and shared LUND infrastructure independent of concrete
 *   event-generator files, schemas and particle records. Each supported generator is implemented by a
 *   small adapter beneath the event-generator-to-LUND converter and is reached only through this
 *   dispatch boundary.
 *
 * Workflow:
 *   The create-lund entry point resolves and validates RunConfig -> convertPhysical reads the resolved
 *   `event-generator` identifier -> the matching adapter reads physical truth and submits common Event
 *   objects to the shared target-vertex and LUND-output components -> the adapter finalizes the run.
 *
 * Inputs:
 *   RunConfig contains the selected event generator plus the common input, target, beam, event-limit,
 *   output and provenance settings. Generator-specific adapters interpret only their own input schema.
 *
 * Outputs:
 *   This dispatcher creates no files itself. The selected synchronous adapter owns conversion for the
 *   call and produces the split LUND files and completion manifest defined by the common workflow.
 *
 * Ownership and lifetime:
 *   The caller owns RunConfig and must keep it alive until convertPhysical returns. The dispatcher and
 *   adapters borrow the immutable configuration; they neither retain nor transfer its ownership.
 *
 * Extension:
 *   Adding another physical format requires one adapter implementing the same resolved-configuration
 *   contract and one explicit dispatch branch. It must reuse common LUND serialization rather than
 *   duplicating file lifecycle, target sampling, naming, splitting, or manifest behavior.
 *
 * Failure:
 *   An unsupported generator is rejected before adapter work begins. Configuration, input, conversion,
 *   target and output failures raised by the selected adapter propagate unchanged to the CLI boundary.
 */

#pragma once
#include "core/config/RunConfig.h"

namespace samples {

// Physical conversion entry point ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Physical conversion entry point */

/**
 * @brief Dispatch configured physical input to its event-generator adapter.
 *
 * Purpose:
 *   Select exactly one physical-input implementation from the resolved `event-generator` value while
 *   keeping format-specific types and logic out of callers and common LUND code.
 *
 * Workflow:
 *   Inspect the resolved generator identifier, invoke its adapter synchronously, return only after that
 *   adapter has completed, and reject any identifier without an implemented branch.
 *
 * Inputs:
 *   config is a caller-owned, immutable physical-source configuration. The current implementation accepts
 *   `genie-gst`; configuration resolution supplies that format-specific value as the default when no
 *   override is provided.
 *
 * Outputs:
 *   No C++ value is returned and the dispatcher itself owns no output state. A successful adapter call
 *   leaves the completed physical LUND run described by that adapter and the shared writer contract.
 *
 * Invariants:
 *   Exactly one adapter is called per invocation. Dispatch does not generate interactions, reinterpret
 *   generator truth, sample uniform kinematics, or automatically submit detector-simulation jobs.
 *
 * Failure:
 *   Throws std::runtime_error when `event-generator` has no implemented adapter. Exceptions from the
 *   selected adapter propagate without being converted into a false successful return.
 *
 * @param config Resolved and validated physical-source run configuration borrowed for the duration of
 *               the synchronous conversion; never modified or retained.
 *
 * @throws std::runtime_error If no adapter implements the configured generator.
 */
void convertPhysical(const RunConfig& config);

#pragma endregion

}  // namespace samples
