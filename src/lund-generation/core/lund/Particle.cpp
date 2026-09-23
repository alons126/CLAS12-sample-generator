//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Particle.cpp
 * @brief Implements the supported LUND particle-mass lookup.
 *
 * Purpose:
 *   Expose the particle masses owned by the protected target source without duplicating them in
 *   maintained code. Event producers select particle identities and momenta; this module supplies only
 *   the corresponding rest mass and does not alter event kinematics.
 *
 * Workflow:
 *   1. A uniform or physical event adapter chooses a supported PDG code.
 *   2. The adapter requests the corresponding targets.h mass.
 *   3. The returned mass is stored in Particle.
 *   4. LundWriter combines that mass with momentum to calculate on-shell energy.
 *
 * Units and conventions:
 *   All returned masses are in GeV/c². Electron, proton, neutron, and charged-pion values come directly
 *   from the protected targets.h source. The photon is exactly massless. Neutral pions are not supported
 *   output particles; physical inputs must provide their upstream-generated decay photons.
 *
 * Failure behavior:
 *   A PDG code outside the supported LUND particle set raises std::runtime_error
 *   instead of silently assigning an unknown or zero mass.
 */

#include "core/geometry/TargetGeometry.h"
#include "core/lund/Event.h"

namespace samples {

// particleMass ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* particleMass */

/**
 * @brief Return the centralized output mass.
 *
 * Purpose:
 *   Translate one supported PDG identity into the rest mass stored in the
 *   corresponding Particle and ultimately written to its LUND record.
 *
 * Algorithm:
 *   Delegate the exact PDG lookup to the target-source adapter so protected values remain hidden and
 *   are included in one translation unit only.
 *
 * @param pid Particle PDG identifier selected by the event-source adapter.
 * @return Rest mass in GeV/c².
 *
 * @throws std::runtime_error If pid is not an electron, proton, neutron,
 *                            charged pion, or photon.
 *
 * @note This function validates identity support only. It does not decide which
 *       particles an adapter retains and does not calculate total energy.
 */
double particleMass(int pid) { return TargetGeometry::mass(pid); }

#pragma endregion

}  // namespace samples
