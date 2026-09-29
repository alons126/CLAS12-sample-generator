//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetGeometry.h
 * @brief Provides event positions inside a target and the particle masses stored in the external targets.h.
 *
 * Purpose:
 *   Every LUND event needs one vertex: the x, y, and z position where the interaction happens inside the
 *   target. TargetGeometry asks the targets.h file to choose that position. It also reads the particle
 *   masses stored in the same file.
 *
 * Workflow:
 *   RunConfig selects a geometry name such as `Ar`. The constructor checks that targets.h contains that
 *   geometry but does not choose a position yet. For each event, sampleVertexPosition() uses the
 *   random-number generator supplied by the caller to choose one position in centimeters. The event
 *   creator gives that same position to every particle in the event. getMass() returns the stored mass for
 *   a supported particle.
 *
 * Scope:
 *   The geometry name controls only where an event may occur. It does not set A or Z in the LUND header.
 *   RunConfig checks that the geometry matches the selected GEMC target setup.
 */

#pragma once

#include <TRandom3.h>
#include <TVector3.h>

#include <string>
#include <utility>

namespace samples {

// Target positions and particle masses ----------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target positions and particle masses */

// TargetGeometry class --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry class */
/**
 * @class TargetGeometry
 * @brief Checks one target geometry and chooses event positions inside it.
 *
 * Purpose:
 *   Give the event creators a simple way to request a valid target position. This class keeps the shared
 *   variables used by targets.h out of the rest of the project. It also provides access to the particle
 *   masses defined there.
 *
 * Creation and lifecycle:
 *   The constructor copies and checks the geometry name. It does not choose a position. Each call to
 *   sampleVertexPosition() uses the caller's TRandom3 object to choose a position. That call advances the
 *   generator, so its next call produces the next values in the same random sequence.
 *
 * Accepted geometry:
 *   The name must match a geometry in targets.h, and that geometry must contain at least one target part.
 *   This class always chooses positions from a geometry; it cannot return one fixed position for every
 *   event.
 *
 * Random-number handling:
 *   targets.h uses one shared random-number generator named `ran`. Before choosing a position, the
 *   implementation copies the caller's generator into `ran`. It copies the updated generator back after
 *   choosing the position. The current event loops use one thread, so there is no competing caller today.
 *   A lock is kept for possible future multithreaded event processing, where it would stop two callers
 *   from mixing their random sequences. TargetGeometry does not create or own a random-number generator.
 *
 * Invariants:
 *   name_ identifies a geometry that exists and is not empty. sampleVertexPosition() returns finite x, y,
 *   and z values measured in centimeters. The event creator must call sampleVertexPosition() once per
 *   event and use the returned position for every particle in that event.
 */
class TargetGeometry {
   public:
    /**
     * @brief Constructor: Create an object that chooses positions from one target geometry.
     * @param name Exact geometry name from targets.h, such as `Ar`. The object stores its own copy.
     * @throws std::runtime_error If targets.h has no geometry with this name or the geometry is empty.
     * @note Creating the object does not choose a position or use a random number.
     */
    explicit TargetGeometry(std::string name) : name_(std::move(name)) { validateGeometryName(name_); }

    /**
     * @brief Check that targets.h contains a usable geometry with this name.
     * @param name Exact geometry name from targets.h.
     * @throws std::runtime_error If targets.h has no geometry with this name or the geometry is empty.
     * @note This check does not choose a position or use a random number. A, Z, and the target-material
     *       name do not select the geometry.
     */
    static void validateGeometryName(const std::string& name);

    /**
     * @brief Get the mass of one particle from targets.h.
     * @param pid Standard PDG integer that identifies an electron, proton, neutron, charged pion, or
     *            photon.
     * @return Particle mass in GeV/c². The electron, proton, neutron, and charged-pion masses come from
     *         targets.h. Photons are massless.
     * @throws std::runtime_error If pid identifies any other particle.
     * @note This function does not use random numbers. It reads the existing targets.h values instead of
     *       keeping another copy of the mass values.
     */
    static double getMass(int pid);

    /**
     * @brief Choose one random x, y, and z position inside the selected target geometry.
     * @param random Generator that supplies the random values. The caller owns it. This call advances it
     *               to the next point in its random sequence.
     * @return The chosen position in centimeters.
     * @throws std::runtime_error If any returned coordinate is infinite or is not a number. Errors
     *                            reported by targets.h are also passed to the caller.
     */
    TVector3 sampleVertexPosition(TRandom3& random) const;

    // Stored geometry name ----------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /** @brief Copy of the checked targets.h geometry name used by sampleVertexPosition(). */
    std::string name_;
};
#pragma endregion

#pragma endregion

}  // namespace samples
