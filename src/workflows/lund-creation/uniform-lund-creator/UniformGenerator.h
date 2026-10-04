/**
 * @file UniformGenerator.h
 * @brief Provides the function that creates one complete uniform LUND sample.
 *
 * Purpose:
 *   A uniform sample sends particles across chosen momentum and angle ranges so the user can study where
 *   CLAS12 detects them. The events are deliberately random test events, not simulated interactions.
 *   generateUniform() creates those events, chooses their vertex positions, writes the LUND
 *   files, and saves plots that show the generated values.
 *
 * Execution flow:
 *   Build and check a RunConfig for the uniform LUND creator, then pass it to generateUniform(). A normal
 *   return means the run directory contains its LUND files, monitoring output, and completion manifest.
 *
 * Scope:
 *   The function creates electron-test, one-electron, or electron-hadron test events from configured
 *   random momentum and angles. It does not model a physical interaction, read event-generator input, run
 *   an external event generator such as GENIE, or submit detector-simulation jobs.
 */

#pragma once

#include "core/config/RunConfig.h"

namespace samples {

// Uniform LUND creation -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Uniform LUND creation */
/**
 * @brief Create the requested uniform test events and all completed-run output.
 *
 * Purpose:
 *   Produce a sample that covers the configured particle ranges and can later be passed to detector
 *   simulation. Use the shared vertex-position rules, LUND writer, output naming, manifest, and completion
 *   marker.
 *
 * Execution flow:
 *   1. Check the final settings and convert them to event-loop values.
 *   2. Start one random-number generator for particle motion and another for vertex positions.
 *   3. Replace and prepare the exact run directory, then create the monitoring plots.
 *   4. Create and write events until the requested count is reached. Every particle in one event receives
 *      the same vertex position.
 *   5. Save the monitoring output, close the LUND files, and publish the completion manifest.
 *
 * @param config Checked settings returned by RunConfig::createFromCommandLine() with
 *               LundSource::Uniform. The function reads but does not change them. Momentum uses GeV/c,
 *               beam energy uses GeV, angles use degrees, and vertex positions use centimeters.
 * @return Nothing. Normal return means all requested events, monitoring output, and the completion
 *         manifest were written.
 * @throws std::exception If setting checks, vertex-position sampling, directory preparation, LUND writing,
 *                        or monitoring output fails.
 * @note A vertex position is the Vx, Vy, and Vz coordinates written to each LUND particle record. Separate random-number
 *       generators ensure that choosing this position does not change the sequence used for particle
 *       momentum and angles.
 * @note Each LUND file holds 25,000 events by default. `events-per-file` can change that limit. The
 *       manifest records the actual count in each file for later simulation submission.
 * @note Theta is chosen with equal probability across its configured angle range. In mixed momentum mode,
 *       even event IDs choose p uniformly and odd event IDs choose 1/p uniformly. Neutrons instead use
 *       uniform p and are the only hadrons allowed to use fixed 1 GeV/c momentum.
 */
void generateUniform(const RunConfig& config);
#pragma endregion

}  // namespace samples
