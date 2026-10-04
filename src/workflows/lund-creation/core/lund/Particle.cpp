/**
 * @file Particle.cpp
 * @brief Gets the mass needed to write a supported particle to a LUND file.
 *
 * Purpose:
 *   Each LUND particle needs a mass so LundWriter can calculate its energy. The uniform LUND creator or
 *   physical LUND converter identifies the particle with its standard PDG integer, such as 11 for an
 *   electron. getParticleMass() returns the matching mass from the external targets.h file. The code
 *   does not keep another copy of those mass values.
 *
 * Execution flow:
 *   The uniform LUND creator or physical LUND converter chooses a supported particle and passes its PDG
 *   integer to getParticleMass(). The function asks TargetGeometry to read the mass from targets.h. The
 *   caller stores the returned mass in Particle, and LundWriter later uses it to calculate the particle's
 *   energy. This function does not choose which particles belong in an event or change their motion.
 *
 * Failure behavior:
 *   If the PDG integer does not identify a supported particle, TargetGeometry reports an error. No mass is
 *   guessed.
 */

#include "core/geometry/TargetGeometry.h"
#include "core/lund/Event.h"

namespace samples {

// Getting a particle mass -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Getting a particle mass */

// TargetGeometry reads the value from targets.h. Calling it here keeps getParticleMass() as the particle
// code's simple entry point without creating a second table of mass values.
double getParticleMass(int pid) { return TargetGeometry::getMass(pid); }

#pragma endregion

}  // namespace samples
