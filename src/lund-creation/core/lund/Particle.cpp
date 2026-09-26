//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Particle.cpp
 * @brief Implements the supported LUND particle-mass lookup.
 *
 * Purpose:
 *   Return particle masses from the external targets.h file without copying those values into maintained
 *   code. This file does not choose particles or change their momenta.
 *
 * Workflow:
 *   A generator or converter chooses a supported PDG number -> particleMass() asks TargetGeometry for
 *   its mass -> the caller stores that mass in Particle -> LundWriter uses it to calculate energy.
 *
 * Failure behavior:
 *   An unsupported PDG number throws std::runtime_error instead of returning a guessed mass.
 */

#include "core/geometry/TargetGeometry.h"
#include "core/lund/Event.h"

namespace samples {

// particleMass ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* particleMass */

// Forward the lookup so targets.h remains the only source of nonzero particle masses.
double particleMass(int pid) { return TargetGeometry::mass(pid); }

#pragma endregion

}  // namespace samples
