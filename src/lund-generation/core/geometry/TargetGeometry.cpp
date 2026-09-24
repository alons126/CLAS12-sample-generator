//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetGeometry.cpp
 * @brief Adapter around the external target source.
 *
 * Purpose:
 *   Use replaceable external targets.h as the authoritative spatial sampler while isolating its
 *   definitions and global RNG behind a maintained, validated, thread-safe interface.
 *
 * Workflow:
 *   Isolate the external header in one private namespace/translation unit -> validate a nonempty map
 *   entry -> lock global sampling state -> copy in the caller-owned TRandom3 -> invoke randomVertex()
 *   -> copy the advanced state back -> reject non-finite vertices.
 *
 * Reproducibility and units:
 *   Callers own and seed vertex RNG streams independently from kinematic RNGs. Copying complete TRandom3
 *   state preserves draw order across interleaved geometry objects. Returned coordinates are cm.
 *
 * External-source rule:
 *   The included external header is consumed as-is. Geometry updates occur by explicitly replacing that
 *   external file, while this adapter remains stable unless its interface changes.
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
 * @brief Translation-unit-private ownership boundary for external symbols and synchronization state.
 *
 * targets.h defines data and functions rather than declarations alone, including a global TRandom3.
 * Keeping its inclusion and the matching mutex here prevents those names from entering the maintained
 * samples namespace and prevents multiple-definition problems in other translation units.
 */
namespace {

// External targets namespace --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* External targets namespace */
/**
 * @namespace external_targets
 * @brief Private namespace containing the unmodified symbols defined by external targets.h.
 *
 * The using-declarations supply standard-library names expected unqualified by the imported header.
 * They remain confined to this private namespace and do not change maintained application APIs.
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
 * @brief Process-lifetime lock protecting the external global RNG transaction.
 *
 * Every sample holds this mutex from caller-state installation through external sampling and
 * state retrieval, so concurrent geometry calls cannot mix streams. The immutable target map needs no
 * mutation lock during validation.
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
    // Use find rather than operator[] so validation cannot insert a missing key into external state.
    const auto found = external_targets::targets.find(name);
    if (found == external_targets::targets.end() || found->second.empty()) { throw std::runtime_error("Unknown or empty target geometry in targets.h: " + name); }
}
#pragma endregion

// TargetGeometry::sample ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry::sample */
TVector3 TargetGeometry::sample(TRandom3& random) const {
    // Upstream randomVertex uses a global TRandom3 named ran. Transfer full state rather than reseeding,
    // so streams remain reproducible and independent when geometry instances are interleaved.
    std::lock_guard<std::mutex> guard(geometry_mutex);
    external_targets::ran = random;
    const auto vertex = external_targets::randomVertex(name_);
    random = external_targets::ran;

    // Mag2 covers all three coordinates in one check; non-finite inputs or overflow are invalid output.
    if (!std::isfinite(vertex.Mag2())) { throw std::runtime_error("Non-finite vertex from targets.h"); }
    return vertex;
}
#pragma endregion

}  // namespace samples
