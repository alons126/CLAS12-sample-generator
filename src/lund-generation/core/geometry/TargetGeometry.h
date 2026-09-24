//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetGeometry.h
 * @brief Read-only adapter for external target geometry and particle masses.
 *
 * Purpose:
 *   Expose the external targets.h geometry catalog and particle masses through a small maintained API
 *   while hiding its global symbols and allowing each run to own an explicit, reproducible vertex-
 *   random stream.
 *
 * Workflow:
 *   RunConfig resolves and validates a geometry key -> construct one TargetGeometry without consuming
 *   random numbers -> pass the run-owned vertex TRandom3 to sample() exactly once per written/retained
 *   event -> assign the returned cm vertex to every particle in that event. The LUND particle adapter
 *   requests supported masses through mass() without exposing or duplicating target-source values.
 *
 * Scope:
 *   This adapter selects spatial distributions only. RG-M target identity, nuclear A/Z header metadata,
 *   and GEMC target variation remain independent configuration inputs.
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
 * @brief Bridge between a run-owned vertex stream and external targets.h.
 *
 * Purpose:
 *   Preserve the external geometry implementation as the authoritative vertex source without exposing
 *   its map or global TRandom3 to generators and converters.
 *
 * Creation and lifecycle:
 *   When created, the object stores the geometry name and checks that it is valid without generating a
 *   vertex. The name does not change afterward. Each sample() call uses and advances the RNG supplied by
 *   the caller.
 *
 * Geometry modes:
 *   The name must match a target with at least one position in external targets.h. There is no option
 *   to use a fixed vertex.
 *
 * RNG ownership and concurrency:
 *   targets.h uses a shared global RNG named `ran`. The implementation locks access, copies the caller's
 *   RNG state into `ran`, samples a vertex, and copies the updated state back. TargetGeometry does not
 *   own an RNG, so each caller keeps its own independent random sequence.
 *
 * Invariants:
 *   name_ is a validated nonempty external map key. Returned vertices use cm and must be
 *   finite. The generator/converter, rather than this object, enforces one sample per event and a shared
 *   vertex for all particles.
 */
class TargetGeometry {
   public:
    /**
     * @brief Store and validate one target-geometry mode without consuming random numbers.
     *
     * @param name External targets.h map key. The string is moved into
     *             owned state after the parameter is copied/moved by the caller.
     *
     * @throws std::runtime_error If name is not a nonempty external geometry entry.
     */
    explicit TargetGeometry(std::string name) : name_(std::move(name)) { validate(name_); }

    /**
     * @brief Check whether a geometry mode can be constructed, without sampling or changing RNG state.
     *
     * @param name External targets.h key.
     *
     * @throws std::runtime_error If the key is absent or its external geometry record is empty.
     *
     * @note Validation does not infer a geometry from A, Z, or an RG-M identifier.
     */
    static void validate(const std::string& name);

    /**
     * @brief Return one supported particle mass from the external target source.
     *
     * @param pid PDG identifier for electron, proton, neutron, charged pion, or photon.
     *
     * @return Mass in GeV/c². Electron, nucleon, and charged-pion values are read from targets.h;
     *         photon mass is exactly zero.
     *
     * @throws std::runtime_error If pid is not part of the maintained LUND particle contract.
     *
     * @note Read-only; consumes no random numbers and owns no duplicate maintained mass table.
     */
    static double mass(int pid);

    /**
     * @brief Produce one finite interaction vertex from the selected geometry.
     *
     * @param random Borrowed run-owned vertex TRandom3. Physical target sampling advances its complete
     *               state.
     *
     * @return Vertex in cm.
     *
     * @throws std::runtime_error If the external sampler returns non-finite coordinates.
     *         Exceptions from the external implementation may also propagate.
     */
    TVector3 sample(TRandom3& random) const;

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /** @brief Validated external map key, owned for object lifetime. */
    std::string name_;
};
#pragma endregion

#pragma endregion

}  // namespace samples
