//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformGenerator.h
 * @brief Declares uniform LUND sample generation.
 *
 * Purpose:
 *   Provide one function that creates unphysical CLAS12 acceptance samples. Sampling,
 *   target vertices, LUND writing, and monitoring stay inside the implementation.
 *
 * Workflow:
 *   Read a uniform RunConfig -> call generateUniform() -> use the LUND files, plots, and run log.
 *
 * Scope:
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
 *   Produce one uniform acceptance run using the shared target, LUND writer, naming, settings record,
 *   and completion marker.
 *
 * Workflow:
 *   1. Check and store the settings.
 *   2. Start separate random streams for particle motion and vertices.
 *   3. Prepare the output directory and plots.
 *   4. Create events until the requested count is written. All particles in one event share one vertex.
 *   5. Save the plots, close the LUND files, and write the completion log.
 *
 * @param config Checked settings returned by RunConfig::parse(..., true). The function reads them during
 *               the call and does not store or change them. Momentum uses GeV/c, beam energy uses GeV,
 *               angles use degrees, and vertices use cm.
 *
 * @return Nothing. Normal return means the events, plots, and completion log were written.
 *
 * @throws std::exception If checking, vertex sampling, output setup, LUND writing, or plot writing fails.
 *
 * @note Every particle in an event gets the same vertex. Separate random streams keep vertex draws from
 *       changing the particle-motion sequence.
 *
 * @note Uniform files default to 25,000 events each. `events-per-file` remains configurable, and the
 *       the completion log supplies each actual file count to job submission.
 *
 * @note Production uses the established flat-theta ranges. The 1e electron and charged hadrons alternate
 *       uniform-p and uniform-1/p events. Neutrons use uniform p; only neutrons may use fixed 1 GeV/c.
 */
void generateUniform(const RunConfig& config);
#pragma endregion

}  // namespace samples
