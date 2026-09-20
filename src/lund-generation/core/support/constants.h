//
// Created by Alon Sportes on 06/05/2025.
//

/**
 * @file constants.h
 * @brief Central particle identities and rest masses used by maintained sample generation.
 *
 * Purpose:
 *   Give uniform generation, physical conversion, monitoring, and LUND serialization one authoritative
 *   source for supported PDG identifiers and masses instead of repeating numeric literals.
 *
 * Conventions:
 *   PDG identifiers follow the standard Monte Carlo particle-numbering scheme. Maintained masses use
 *   rounded 2026 Particle Data Group values in GeV/c². The electron is intentionally massless in the
 *   LUND acceptance-sample convention.
 *
 * Scope:
 *   This header describes particles accepted by the shared Event/LUND contract. Detector acceptance,
 *   sampling thresholds, target geometry, and event-generator selection belong to their own modules.
 */

#pragma once

namespace samples::constants {

// Supported particle identities -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Supported particle identities */

/** @enum Pdg @brief Named PDG identifiers for every species supported by the maintained LUND writer. */
enum class Pdg : int {
    electron = 11,
    photon = 22,
    pi_zero = 111,
    pi_plus = 211,
    pi_minus = -211,
    neutron = 2112,
    proton = 2212,
};

constexpr int electron_pdg = static_cast<int>(Pdg::electron);  ///< Electron identifier.
constexpr int photon_pdg = static_cast<int>(Pdg::photon);      ///< Photon identifier.
constexpr int pi_zero_pdg = static_cast<int>(Pdg::pi_zero);    ///< Neutral-pion identifier.
constexpr int pi_plus_pdg = static_cast<int>(Pdg::pi_plus);    ///< Positive-pion identifier.
constexpr int pi_minus_pdg = static_cast<int>(Pdg::pi_minus);  ///< Negative-pion identifier.
constexpr int neutron_pdg = static_cast<int>(Pdg::neutron);    ///< Neutron identifier.
constexpr int proton_pdg = static_cast<int>(Pdg::proton);      ///< Proton identifier.

#pragma endregion

// PDG 2026 rest masses --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* PDG 2026 rest masses */

/**
 * @namespace mass
 * @brief Maintained rest masses from the Particle Data Group 2026 Review of Particle Physics.
 *
 * Values are converted from MeV/c² to GeV/c² and rounded to the five decimal places serialized in the
 * LUND particle record. The charged-pion value applies to both charges. The electron and photon are
 * massless in this event-record model.
 *
 * @see https://pdg.lbl.gov/2026/listings/particle_properties.html
 */
namespace mass {
constexpr double electron = 0.0;
constexpr double proton = 0.93827;
constexpr double neutron = 0.93957;
constexpr double pi_charged = 0.13957;
constexpr double pi_zero = 0.13498;
constexpr double photon = 0.0;
}  // namespace mass

#pragma endregion

}  // namespace samples::constants
