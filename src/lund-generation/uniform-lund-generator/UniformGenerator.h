//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformGenerator.h
 * @brief Declares uniform LUND sample generation.
 *
 * Purpose:
 *   Provide one function that creates deliberately unphysical CLAS12 acceptance samples. Sampling,
 *   target vertices, LUND writing, and monitoring stay inside the implementation.
 *
 * Workflow:
 *   Parse a uniform RunConfig -> call generateUniform() once -> consume the completed LUND files,
 *   monitoring files, and manifest from the resolved run directory.
 *
 * Scientific scope:
 *   The code generates configured 1e or electron-hadron probes from random kinematics. It is not a
 *   physics interaction model and does not read or run an event generator. Physical event conversion
 *   uses the separate physical workflow.
 */

#pragma once
#include "core/config/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */
/**
 * @brief Generate the requested acceptance channel and its completed run outputs.
 *
 * Purpose:
 *   Produce one uniform acceptance run using the shared target geometry, LUND writer, naming, settings
 *   record, and completion marker.
 *
 * Workflow:
 *   1. Revalidate and cache the uniform configuration.
 *   2. Initialize independent kinematic and vertex random streams from their configured seeds.
 *   3. Safely prepare the resolved output directory and initialize uniform monitoring.
 *   4. Generate 1e events or artificial trigger-electron+hadron events until the requested
 *      event capacity is written, sampling exactly one shared vertex per event.
 *   5. Save modern monitoring plus the archived prefix-based ROOT/PDF/PNG artifacts, retain the stable
 *      single `<prefix>_monitoring_plots.root` file, finalize LUND output, and publish the completion
 *      log in the same monitoring directory.
 *
 * @param config Checked settings returned by RunConfig::parse(..., true). The function reads them during
 *               the call and does not store or change them. Momentum uses GeV/c, beam energy uses GeV,
 *               angles use degrees, and vertices use cm.
 *
 * @return Nothing. Normal return means the requested events, monitoring files, and completion manifest
 *         were written successfully.
 *
 * @throws std::exception If validation, target sampling, safe output preparation, LUND serialization,
 *         monitoring output, or manifest publication fails. The CLI boundary converts the exception
 *         to an error diagnostic and nonzero process status.
 *
 * @note Every particle in an electron-hadron event gets the same sampled vertex. Separate RNGs keep
 *       vertex draws from changing the kinematic random sequence.
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
