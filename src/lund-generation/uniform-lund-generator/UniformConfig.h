//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformConfig.h
 * @brief Stores uniform-generator settings in their C++ types.
 *
 * Purpose:
 *   Convert the final RunConfig text into numbers, flags, and enums before the event loop.
 *
 * Workflow:
 *   RunConfig checks the text settings -> UniformConfig converts them -> UniformGenerator uses them.
 *
 * Scope:
 *   This object stores particle and kinematic settings only. Output, formatting, RNG seeds, and target
 *   geometry remain in RunConfig.
 */

#pragma once
#include "core/config/RunConfig.h"
#include "core/lund/Event.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// UniformChannel object -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformChannel object */
/**
 * @enum UniformChannel
 * @brief Event type selected from the checked channel setting.
 *
 * Purpose:
 *   Let the event loop choose an event type without comparing text for every event.
 *
 * Creation and use:
 *   UniformConfig converts the checked channel string to this enum. UniformGenerator uses it to choose
 *   the number of particles and their sampling rules.
 *
 * Meaning:
 *   Electron writes one sampled electron. ElectronHadron writes a trigger electron followed
 *   by the separately selected proton, neutron, pi+, or pi-. Both are acceptance probes.
 */
enum class UniformChannel {
    Electron,        ///< `1e`: one generated electron.
    ElectronTester,  ///< `electron-tester`: one beam-momentum electron for the angular scan.
    ElectronHadron,  ///< `eh`: trigger electron followed by the selected hadron.
};
#pragma endregion

// HadronSpecies object --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* HadronSpecies object */
/**
 * @enum HadronSpecies
 * @brief Hadron selected by `--hadron` for an electron-hadron sample.
 *
 * Creation and use:
 *   UniformConfig converts the checked `--hadron` text to one value. The generator uses that value
 *   with hadron_pid to choose the particle written after the trigger electron.
 */
enum class HadronSpecies {
    Proton,   ///< Proton selected by `--hadron proton`.
    Neutron,  ///< Neutron selected by `--hadron neutron`.
    PiPlus,   ///< Positive pion selected by `--hadron pip`.
    PiMinus,  ///< Negative pion selected by `--hadron pim`.
};
#pragma endregion

// Resolve string settings once, outside the production event loop.

// UniformConfig object --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformConfig object */
/**
 * @struct UniformConfig
 * @brief Converted sampling settings used in the event loop.
 *
 * Purpose:
 *   Store the converted values needed to choose particles and sample their kinematics. It does not own
 *   the full run configuration or any changing generator state.
 *
 * Creation and lifetime:
 *   Construct once from a checked RunConfig after `auto` and `sampled` have been replaced. Every value
 *   is copied, so this object does not depend on the RunConfig after construction.
 *
 * Units:
 *   Beam energy is GeV, momenta are GeV/c, and all angles are degrees. UniformGenerator converts angles
 *   to radians only at ROOT TVector3/math boundaries. A and Z are dimensionless LUND header metadata.
 *
 * Rules:
 *   Exactly one channel enum is selected. Electron momentum is uniform-p, mixed p and 1/p, or beam-valued.
 *   Hadron momentum is exactly one of uniform-p, mixed p and 1/p, or fixed (both flags false). Hadron
 *   theta and phi are always uniform inside their configured bounds. Target metadata and mode/channel
 *   compatibility were checked by RunConfig::validate(true).
 */
struct UniformConfig {
    // Channel and sampling choices --------------------------------------------------------------------------------------------------------------------------------------
    UniformChannel channel;          ///< Electron-only or electron-hadron branch.
    HadronSpecies hadron;            ///< Hadron identity used by the eh branch.
    int hadron_pid;                  ///< Centralized PDG identifier for the selected hadron.
    bool uniform_electron_momentum;  ///< True: p~U(electron_p_min,electron_p_max).
    bool mixed_electron_momentum;    ///< True: 1e alternates uniform-p and uniform-1/p by event ID.
    bool uniform_hadron_momentum;    ///< True: p~U(p_min,p_max); mutually exclusive with mixed mode.
    bool mixed_hadron_momentum;      ///< True: charged hadrons alternate uniform-p and uniform-1/p by event ID.

    // Physical ranges and trigger settings ------------------------------------------------------------------------------------------------------------------------------
    double beam;                ///< Beam energy in GeV; also supplies the c=1 momentum-scale value.
    double electron_theta_min;  ///< Inclusive lower electron polar-angle bound in degrees.
    double electron_theta_max;  ///< Upper electron polar-angle bound in degrees.
    double electron_p_min;      ///< Lower sampled electron-momentum bound in GeV/c.
    double electron_p_max;      ///< Upper sampled electron-momentum bound in GeV/c.
    double hadron_theta_min;    ///< Inclusive lower hadron polar-angle bound in degrees.
    double hadron_theta_max;    ///< Upper hadron polar-angle bound in degrees.
    double hadron_p;            ///< Fixed-mode neutron momentum in GeV/c.
    double hadron_p_min;        ///< Lower sampled-momentum bound in GeV/c.
    double hadron_p_max;        ///< Upper sampled-momentum bound in GeV/c.
    double trigger_theta;       ///< Artificial trigger-electron polar angle in degrees.
    double trigger_phi_offset;  ///< Offset from the selected opposite-sector center in degrees.
    int A;                      ///< LUND target mass-number metadata; does not select geometry.
    int Z;                      ///< LUND target charge-number metadata; does not select geometry.

    /**
     * @brief Copy and convert one checked uniform configuration.
     *
     * @param c Checked uniform settings with all automatic values already replaced.
     *
     * @throws std::exception If a required setting is missing or a number cannot be read. This constructor
     *         does not check or repair the settings.
     *
     * @note After validation, the only remaining channel is `eh`, and only four hadron names are allowed.
     */
    explicit UniformConfig(const RunConfig& c)
        : channel(c.get("channel") == "1e"                ? UniformChannel::Electron
                  : c.get("channel") == "electron-tester" ? UniformChannel::ElectronTester
                                                          : UniformChannel::ElectronHadron),
          hadron(c.get("hadron") == "proton"    ? HadronSpecies::Proton
                 : c.get("hadron") == "neutron" ? HadronSpecies::Neutron
                 : c.get("hadron") == "pip"     ? HadronSpecies::PiPlus
                                                : HadronSpecies::PiMinus),
          hadron_pid(c.get("hadron") == "proton"    ? constants::proton_pdg
                     : c.get("hadron") == "neutron" ? constants::neutron_pdg
                     : c.get("hadron") == "pip"     ? constants::pi_plus_pdg
                                                    : constants::pi_minus_pdg),
          uniform_electron_momentum(c.get("electron-momentum") == "uniform"),
          mixed_electron_momentum(c.get("electron-momentum") == "mixed"),
          uniform_hadron_momentum(c.get("hadron-momentum") == "uniform"),
          mixed_hadron_momentum(c.get("hadron-momentum") == "mixed"),
          beam(c.number("beam-energy")),
          electron_theta_min(c.number("electron-theta-min")),
          electron_theta_max(c.number("electron-theta-max")),
          electron_p_min(c.number("electron-p-min")),
          electron_p_max(c.number("electron-p-max")),
          hadron_theta_min(c.number("hadron-theta-min")),
          hadron_theta_max(c.number("hadron-theta-max")),
          hadron_p(c.number("hadron-p")),
          hadron_p_min(c.number("hadron-p-min")),
          hadron_p_max(c.number("beam-energy")),
          trigger_theta(c.number("trigger-theta")),
          trigger_phi_offset(c.number("trigger-phi-offset")),
          A(static_cast<int>(c.integer("A"))),
          Z(static_cast<int>(c.integer("Z"))) {}
};
#pragma endregion

#pragma endregion

}  // namespace samples
