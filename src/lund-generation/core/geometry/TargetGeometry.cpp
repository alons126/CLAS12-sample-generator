//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetGeometry.cpp
 * @brief Implements checked access to the external targets.h file.
 *
 * Purpose:
 *   Use targets.h for vertex sampling and particle masses while keeping its global variables inside
 *   this source file. A mutex makes use of its global random-number generator safe between threads.
 *
 * Workflow:
 *   Include targets.h inside a private namespace -> check that a geometry exists -> lock its global RNG
 *   -> copy in the caller's TRandom3 state -> call randomVertex() -> copy the new state back -> reject a
 *   vertex with non-finite coordinates.
 *
 * Reproducibility and units:
 *   The caller owns and seeds the vertex RNG separately from the kinematic RNG. Copying the complete
 *   TRandom3 state keeps each caller's random sequence independent. Vertex coordinates are in cm.
 *
 * External-source rule:
 *   This file reads the external header without changing it. Updating target definitions means replacing
 *   that external file; this maintained adapter changes only if the external interface changes.
 */

#include "core/geometry/TargetGeometry.h"

#include <TString.h>

#include <cmath>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "core/lund/Event.h"

// External geometry bridge ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* External geometry bridge */
/**
 * @brief Keeps targets.h names and their mutex private to this source file.
 *
 * targets.h defines global data and functions, including a TRandom3. Including it only here keeps those
 * names out of the public samples namespace and avoids defining them in more than one source file.
 */
namespace {

// External targets namespace --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* External targets namespace */
/**
 * @namespace external_targets
 * @brief Holds the unchanged names defined by external targets.h.
 *
 * The using declarations provide standard-library names that targets.h expects. They stay inside this
 * private namespace and do not become part of the maintained API.
 */
namespace external_targets {
using std::cout;
using std::endl;
using std::sqrt;
using std::string;
#include "external/targets.h"

}  // namespace external_targets
#pragma endregion

/**
 * @brief Lock used whenever code samples with the global RNG in targets.h.
 *
 * A sample holds this mutex while it copies in the caller's state, draws a vertex, and copies the state
 * back. This prevents two threads from mixing their random sequences. Validation only reads the target
 * map and does not need this lock.
 */
std::mutex geometry_mutex;
}  // namespace
#pragma endregion

namespace samples {

// TargetGeometry::mass --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry::mass */
double TargetGeometry::mass(int pid) {
    switch (pid) {
        case constants::electron_pdg:
            return external_targets::mass_e;
        case constants::proton_pdg:
            return external_targets::mass_p;
        case constants::neutron_pdg:
            return external_targets::mass_n;
        case constants::pi_plus_pdg:
        case constants::pi_minus_pdg:
            return external_targets::mass_pi;
        case constants::photon_pdg:
            return 0.;
        default:
            throw std::runtime_error("Unsupported output PDG code: " + std::to_string(pid));
    }
}
#pragma endregion

// TargetGeometry::validate ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry::validate */
void TargetGeometry::validate(const std::string& name) {
    // find() checks the map without adding a missing name.
    const auto found = external_targets::targets.find(name);
    if (found == external_targets::targets.end() || found->second.empty()) { throw std::runtime_error("Unknown or empty target geometry in targets.h: " + name); }
}
#pragma endregion

// TargetGeometry::sample ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry::sample */
TVector3 TargetGeometry::sample(TRandom3& random) const {
    // randomVertex() uses the global RNG named ran. Copy the full state instead of only setting a seed,
    // so each caller continues its own random sequence.
    std::lock_guard<std::mutex> guard(geometry_mutex);
    external_targets::ran = random;
    const auto vertex = external_targets::randomVertex(name_);
    random = external_targets::ran;

    // Mag2() checks all three coordinates at once. A non-finite result is not a valid vertex.
    if (!std::isfinite(vertex.Mag2())) { throw std::runtime_error("Non-finite vertex from targets.h"); }
    return vertex;
}
#pragma endregion

}  // namespace samples
