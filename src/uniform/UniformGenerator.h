//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformGenerator.h
 * @brief Uniform generation entry-point contract.
 *
 * Purpose:
 *   Expose deliberately unphysical CLAS12 acceptance-sample production through one small maintained
 *   API while keeping channel sampling, target vertices, LUND writing, and monitoring internals private.
 *
 * Workflow:
 *   Parse a uniform RunConfig -> call generateUniform() once -> consume the completed LUND files,
 *   monitoring files, and manifest from the resolved run directory.
 *
 * Scientific scope:
 *   The implementation generates configured 1e or electron–hadron probes from random kinematics. It is not an
 *   interaction model and does not read or run an event generator. Physical truth conversion enters
 *   through the separate physical adapter workflow.
 */

#pragma once
#include "config/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */
/**
 * @brief Generate the requested acceptance channel and its completed run outputs.
 *
 * Purpose:
 *   Produce one reproducible uniform acceptance run using the same target geometry, LUND serializer,
 *   naming, provenance, and completion contract used by physical conversion where semantics coincide.
 *
 * Workflow:
 *   1. Revalidate and cache the uniform configuration.
 *   2. Initialize independent kinematic and vertex random streams from their configured seeds.
 *   3. Safely prepare the resolved output directory and initialize both monitoring systems.
 *   4. Generate 1e events or artificial trigger-electron+hadron events until the requested
 *      event capacity is written, sampling exactly one shared vertex per event.
 *   5. Save modern monitoring plus the archived prefix-based ROOT/PDF/PNG artifacts, retain the stable
 *      legacy_histograms.root copy, finalize LUND output, and publish the manifest.
 *
 * @param config Borrowed configuration returned by RunConfig::parse(..., true). The function reads it
 *               for the duration of the call and neither retains nor modifies it. Momentum values
 *               are GeV/c, beam energy is GeV, angles are degrees, and vertex coordinates are cm.
 *
 * @return Nothing. Normal return means the requested events and diagnostics were written and the run
 *         was made consumable through its completion manifest.
 *
 * @throws std::exception If validation, target sampling, safe output preparation, LUND serialization,
 *         monitoring output, or manifest publication fails. The CLI boundary converts the exception
 *         to an error diagnostic and nonzero process status.
 *
 * @note Every particle in an electron–hadron event receives the same sampled interaction vertex. Kinematic and
 *       vertex RNG ownership remains separate so target draws do not change the kinematic sequence.
 *
 * @note Uniform files default to 25,000 events each. `events-per-file` remains configurable, and the
 *       completed manifest supplies each actual file count to GEMC and reconstruction submission.
 *
 * @note Production defaults retain flat-theta legacy angular windows. The 1e electron and charged hadrons alternate uniform-p and uniform-1/p events over their particle-specific bounds.
 * Neutrons sample p uniformly from zero; fixed 1 GeV/c remains neutron-only.
 */
void generateUniform(const RunConfig& config);
#pragma endregion

}  // namespace samples
