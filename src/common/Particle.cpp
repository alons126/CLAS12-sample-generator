//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Particle.cpp
 * @brief Implements the supported LUND particle-mass lookup.
 *
 * Purpose:
 *   Keep the mass constants used in output records in one small, explicit table.
 *   Event producers select particle identities and momenta; this module supplies
 *   only the corresponding rest mass and does not alter event kinematics.
 *
 * Workflow:
 *   1. A uniform or physical event adapter chooses a supported PDG code.
 *   2. The adapter requests either the legacy-compatible or standard pion mass.
 *   3. The returned mass is stored in Particle.
 *   4. LundWriter combines that mass with momentum to calculate on-shell energy.
 *
 * Units and conventions:
 *   All returned masses are in GeV/c². The legacy switch changes only charged
 *   and neutral pion constants; electron, proton, neutron and photon values are
 *   identical in both modes. The photon is treated as massless.
 *
 * Failure behavior:
 *   A PDG code outside the supported LUND particle set raises std::runtime_error
 *   instead of silently assigning an unknown or zero mass.
 */

#include "common/Event.h"
#include <stdexcept>
#include <string>

namespace samples {

// particleMass ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* particleMass */

/**
 * @brief Return the selected output mass convention.
 *
 * Purpose:
 *   Translate one supported PDG identity into the rest mass stored in the
 *   corresponding Particle and ultimately written to its LUND record.
 *
 * Algorithm:
 *   Dispatch on the exact PDG code. Electron, nucleon and photon branches return
 *   their fixed values. Charged pions share one branch, while the neutral pion
 *   has its own standard value. When legacy is true, both pion branches use the
 *   archived common value required for byte-compatible legacy output.
 *
 * @param pid Particle PDG identifier selected by the event-source adapter.
 * @param legacy If true, preserve the archived charged/neutral pion mass value;
 *               otherwise use the maintained standard pion constants.
 *
 * @return Rest mass in GeV/c².
 *
 * @throws std::runtime_error If pid is not an electron, proton, neutron,
 *                            charged pion, neutral pion, or photon.
 *
 * @note This function validates identity support only. It does not decide which
 *       particles an adapter retains and does not calculate total energy.
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

}  // namespace samples
