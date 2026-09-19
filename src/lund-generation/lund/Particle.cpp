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
 *   2. The adapter requests the centralized rounded LUND mass.
 *   3. The returned mass is stored in Particle.
 *   4. LundWriter combines that mass with momentum to calculate on-shell energy.
 *
 * Units and conventions:
 *   All returned masses are in GeV/c² and come from constants.h. The electron and photon are treated
 *   as massless; other supported values are rounded from the current PDG table.
 *
 * Failure behavior:
 *   A PDG code outside the supported LUND particle set raises std::runtime_error
 *   instead of silently assigning an unknown or zero mass.
 */

#include <stdexcept>
#include <string>

#include "lund/Event.h"
#include "support/constants.h"

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
 *   Dispatch on the exact PDG code. Electron, nucleon and photon branches return
 *   their fixed values. Charged pions share one branch, while the neutral pion has its own value.
 *
 * @param pid Particle PDG identifier selected by the event-source adapter.
 * @return Rest mass in GeV/c².
 *
 * @throws std::runtime_error If pid is not an electron, proton, neutron,
 *                            charged pion, neutral pion, or photon.
 *
 * @note This function validates identity support only. It does not decide which
 *       particles an adapter retains and does not calculate total energy.
 */
double particleMass(int pid) {
    switch (pid) {
        case constants::electron_pdg:
            return constants::mass::electron;
        case constants::proton_pdg:
            return constants::mass::proton;
        case constants::neutron_pdg:
            return constants::mass::neutron;
        case constants::pi_plus_pdg:
        case constants::pi_minus_pdg:
            return constants::mass::pi_charged;
        case constants::pi_zero_pdg:
            return constants::mass::pi_zero;
        case constants::photon_pdg:
            return constants::mass::photon;
        default:
            throw std::runtime_error("Unsupported output PDG code: " + std::to_string(pid));
    }
}

#pragma endregion

}  // namespace samples
