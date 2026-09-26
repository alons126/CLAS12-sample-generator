//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetGeometry.h
 * @brief Public access to target vertices and particle masses from targets.h.
 *
 * Purpose:
 *   Let maintained code use the external targets.h file without exposing its global variables. Each run
 *   supplies its own random-number generator for vertex sampling.
 *
 * Workflow:
 *   RunConfig chooses a geometry name -> TargetGeometry checks the name without sampling -> sample()
 *   uses the run's TRandom3 once per event -> every particle in that event receives the returned vertex.
 *   The mass() function reads supported particle masses from the same external file.
 *
 * Scope:
 *   The geometry name controls only where vertices are sampled. RG-M target identity, A and Z header
 *   values, and the GEMC target variation are separate settings.
 */

#pragma once
#include <TRandom3.h>
#include <TVector3.h>

#include <string>
#include <utility>

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// TargetGeometry object -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* TargetGeometry object */
/**
 * @class TargetGeometry
 * @brief Uses targets.h with a random-number stream owned by the caller.
 *
 * Purpose:
 *   Provide checked vertex sampling and mass lookup while keeping the targets.h map and global TRandom3
 *   out of the rest of the maintained code.
 *
 * Creation and lifecycle:
 *   The constructor stores and checks the geometry name without drawing a random value. Each sample()
 *   call uses and advances the TRandom3 supplied by the caller.
 *
 * Geometry modes:
 *   The name must match a nonempty geometry in external targets.h. This class has no fixed-vertex mode.
 *
 * RNG ownership and concurrency:
 *   targets.h uses one global RNG named `ran`. The implementation locks it, copies in the caller's RNG
 *   state, samples the vertex, and copies the new state back. The class does not own an RNG.
 *
 * Invariants:
 *   name_ is a valid, nonempty geometry key. Returned coordinates are finite and measured in cm. The
 *   generator or converter is responsible for sampling once and sharing that vertex across the event.
 */
class TargetGeometry {
   public:
    /**
     * @brief Store and check one geometry name without drawing a random value.
     *
     * @param name Geometry key from external targets.h. The object stores its own string.
     *
     * @throws std::runtime_error If name is missing or its geometry has no entries.
     */
    explicit TargetGeometry(std::string name) : name_(std::move(name)) { validate(name_); }

    /**
     * @brief Check a geometry name without sampling or changing random state.
     *
     * @param name External targets.h key.
     *
     * @throws std::runtime_error If the key is missing or its geometry has no entries.
     *
     * @note A, Z, and the RG-M target name do not choose the geometry here.
     */
    static void validate(const std::string& name);

    /**
     * @brief Return the targets.h mass for one supported particle.
     *
     * @param pid PDG identifier for electron, proton, neutron, charged pion, or photon.
     *
     * @return Mass in GeV/c². Electron, nucleon, and charged-pion values are read from targets.h;
     *         photon mass is exactly zero.
     *
     * @throws std::runtime_error If pid is not part of the maintained LUND particle contract.
     *
     * @note This function does not draw random numbers or keep a second mass table.
     */
    static double mass(int pid);

    /**
     * @brief Sample one finite interaction vertex from the selected geometry.
     *
     * @param random Random-number generator owned by the caller. Sampling advances its state.
     *
     * @return Vertex in cm.
     *
     * @throws std::runtime_error If targets.h returns non-finite coordinates. Other exceptions from
     *         targets.h are passed to the caller.
     */
    TVector3 sample(TRandom3& random) const;

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /** @brief Checked targets.h geometry key stored for the life of this object. */
    std::string name_;
};
#pragma endregion

#pragma endregion

}  // namespace samples
