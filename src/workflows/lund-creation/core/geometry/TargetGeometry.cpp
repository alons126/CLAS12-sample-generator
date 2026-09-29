//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetGeometry.cpp
 * @brief Gets target positions and particle masses from the external targets.h file.
 *
 * Purpose:
 *   Every event needs one x, y, and z position inside the selected target. targets.h contains the target
 *   shapes, the code that chooses a position, and the particle masses. This file gives TargetGeometry
 *   access to those features while keeping the shared targets.h variables out of the rest of the project.
 *
 * Workflow:
 *   validateGeometryName() checks that targets.h contains the requested geometry. The uniform LUND creator
 *   and physical LUND converter call sampleVertexPosition() from one thread, so no competing thread exists
 *   today. The function still locks access to the shared targets.h random-number generator so this boundary
 *   remains safe if event processing becomes multithreaded later. It copies in the caller's generator, asks
 *   targets.h for a position, and copies the updated generator back. It rejects a position containing an
 *   infinite value or a value that is not a number.
 *
 * Reproducibility and units:
 *   The caller creates and seeds the generator used for target positions. This generator is separate from
 *   the one used to create particle motion in the uniform LUND creator. Copying all information that
 *   determines the next random values lets each caller continue its own sequence. Returned positions are
 *   measured in centimeters.
 *
 * targets.h ownership:
 *   This file uses targets.h without changing it. Target shapes and particle masses are updated in that
 *   external file. This file needs a code change only when the way targets.h is called changes.
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

// Private access to targets.h -------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private access to targets.h */
/**
 * @namespace <anonymous>
 * @brief Keeps targets.h and its thread lock available only inside this source file.
 *
 * targets.h creates shared variables and functions when it is included. Including it here, inside this
 * private namespace, prevents the rest of the project from using those details directly. It also prevents
 * the same variables and functions from being created in more than one source file.
 */
namespace {

// Names from targets.h --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Names from targets.h */
/**
 * @namespace external_targets
 * @brief Gives the variables and functions from targets.h their own private name group.
 *
 * targets.h uses `cout`, `endl`, `sqrt`, and `string` without the usual `std::` prefix. These using
 * declarations provide those exact names before the file is included. Keeping everything inside
 * external_targets prevents its names from becoming part of the project's public interface.
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
 * @brief Allows only one thread at a time to use the shared random-number generator in targets.h.
 *
 * The uniform LUND creator and physical LUND converter currently use single-threaded event loops, so this
 * mutex has no competing caller today. It is kept because targets.h exposes one shared generator. If event
 * processing becomes multithreaded later, the mutex prevents simultaneous calls from replacing each other's
 * generator state.
 *
 * Without this lock, one thread could replace the generator while another thread is still choosing a
 * position. Their random sequences would then become mixed. Checking whether a geometry name exists only
 * reads the target list, so that check does not need this lock.
 */
std::mutex geometry_mutex;
}  // namespace
#pragma endregion

namespace samples {

// Particle mass lookup --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Particle mass lookup */
double TargetGeometry::getMass(int pid) {
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

// Geometry name check ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Geometry name check */
void TargetGeometry::validateGeometryName(const std::string& name) {
    // find() looks for the name without accidentally adding an empty geometry when the name is missing.
    const auto found = external_targets::targets.find(name);
    if ((found == external_targets::targets.end()) || found->second.empty()) { throw std::runtime_error("Unknown or empty target geometry in targets.h: " + name); }
}
#pragma endregion

// Event-position sampling -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Event-position sampling */
TVector3 TargetGeometry::sampleVertexPosition(TRandom3& random) const {
    // std::lock_guard<std::mutex> is an object that manages a std::mutex lock. `guard` is this object's
    // local name, and `(geometry_mutex)` tells its constructor which mutex to lock. Construction locks the
    // mutex here; destruction unlocks it when this function returns or throws. If another thread already
    // holds the lock, this thread waits on this line until that thread unlocks it.
    std::lock_guard<std::mutex> guard(geometry_mutex);

    // targets.h always reads random values from its shared generator named `ran`; it cannot accept the
    // caller's `random` generator as an argument. The first assignment copies the caller's current random
    // state into `ran`. randomVertex() uses that state to choose a position for `name_` and advances `ran`.
    // The final assignment copies the advanced state back so the caller's next use continues the sequence.
    external_targets::ran = random;
    const auto vertex = external_targets::randomVertex(name_);
    random = external_targets::ran;

    // Mag2() uses x, y, and z in one calculation. If any coordinate is infinite or is not a number, its
    // result is also invalid, so one check rejects the complete position.
    if (!std::isfinite(vertex.Mag2())) { throw std::runtime_error("Non-finite vertex from targets.h"); }

    return vertex;
}
#pragma endregion

}  // namespace samples
