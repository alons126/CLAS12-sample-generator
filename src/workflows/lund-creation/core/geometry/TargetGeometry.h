/**
 * @file TargetGeometry.h
 * @brief Provides LUND vertex positions and particle masses stored in the external targets.h.
 *
 * Purpose:
 *   Every LUND particle record stores vertex x, y, and z coordinates in centimeters. TargetGeometry asks
 *   targets.h to choose one vertex position per event, and the event creator copies it to every particle
 *   in that event. This class also reads the particle masses stored in the same file.
 *
 * Execution flow:
 *   RunConfig selects a geometry name such as `Ar`. The constructor checks that targets.h contains that
 *   geometry but does not choose a vertex position yet. For each event, sampleVertexPosition() uses the
 *   random-number generator supplied by the caller. targets.h samples Vx and Vy independently from
 *   Gaussian beam-spot distributions. For `liquid` and `Ar`, it samples Vz uniformly across the target
 *   cell. For every other geometry, it places Vz at one listed target-component center; `4-foil` chooses
 *   one of its four foil centers with equal probability. getMass() returns the stored mass for a supported
 *   particle.
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

// Vertex positions and particle masses ----------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Vertex positions and particle masses */

// TargetGeometry class --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry class */
/**
 * @class TargetGeometry
 * @brief Checks one target geometry and chooses LUND vertex positions from it.
 *
 * Purpose:
 *   Give the event creators a simple way to request a valid vertex position. This class keeps the shared
 *   variables used by targets.h out of other code. It also provides access to the particle
 *   masses defined there.
 *
 * Creation and lifecycle:
 *   The constructor copies and checks the geometry name. It does not choose a vertex position. Each call
 *   to sampleVertexPosition() uses the caller's TRandom3 object to choose a vertex position. That call
 *   advances the generator, so its next call produces the next values in the same random sequence.
 *
 * Accepted geometry:
 *   The name must match a geometry in targets.h, and that geometry must contain at least one target part.
 *   Vx and Vy follow the Gaussian beam spot in targets.h. The `liquid` and `Ar` geometries sample Vz
 *   uniformly across their target cells. Other geometries use a listed target-component center for Vz.
 *
 * Random-number handling:
 *   targets.h uses one shared random-number generator named `ran`. Before choosing a vertex position, the
 *   implementation copies the caller's generator into `ran`. It copies the updated generator back after
 *   choosing the vertex position. The current event loops use one thread, so there is no competing caller today.
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
     * @brief Constructor: Create an object that chooses vertex positions from one target geometry.
     * @param name Exact geometry name from targets.h, such as `Ar`. The object stores its own copy.
     * @throws std::runtime_error If targets.h has no geometry with this name or the geometry is empty.
     * @note Creating the object does not choose a vertex position or use a random number.
     */
    explicit TargetGeometry(std::string name) : name_(std::move(name)) { validateGeometryName(name_); }

    /**
     * @brief Check that targets.h contains a usable geometry with this name.
     * @param name Exact geometry name from targets.h.
     * @throws std::runtime_error If targets.h has no geometry with this name or the geometry is empty.
     * @note This check does not choose a vertex position or use a random number. A, Z, and the target-material
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
     * @brief Choose one LUND vertex position from the selected target geometry.
     * @param random Generator that supplies the random values. The caller owns it. This call advances it
     *               to the next point in its random sequence.
     * @return Vertex coordinates in centimeters. Vx and Vy are Gaussian. Vz is uniform across a `liquid`
     *         or `Ar` cell, or is a listed target-component center for every other geometry.
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
