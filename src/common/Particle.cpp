/**
 * @file Particle.cpp
 * @brief Output particle mass conventions.
 *
 * Purpose:
 *   Supply supported PDG masses independently of external geometry and text formatting.
 *
 * Workflow:
 *   Generators request masses; LundWriter combines mass and momentum to compute energy.
 */

#include "common/Event.h"
#include <stdexcept>
#include <string>
namespace samples {
// particleMass ----------------------------------------------------------------------

#pragma region /* particleMass */
/**
 * @brief Return the selected output mass convention.
 *
 * Algorithm:
 *   Select the supported PDG species; apply legacy or standard pion constants.
 *
 * @param pid Particle PDG identifier.
 * @param legacy True to preserve the archived pion masses.
 *
 * @return Mass in GeV; unsupported species throw.
 */
double particleMass(int pid, bool legacy) {
    switch (pid) {
        case 11:
            return 0.000511;
        case 2212:
            return 0.938272;
        case 2112:
            return 0.93957;
        case 211:
        case -211:
            return legacy ? 0.13957 : 0.13957039;
        case 111:
            return legacy ? 0.13957 : 0.1349768;
        case 22:
            return 0;
        default:
            throw std::runtime_error("Unsupported output PDG code: " + std::to_string(pid));
    }
}
#pragma endregion

}
