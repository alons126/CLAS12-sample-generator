//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformConfig.h
 * @brief Typed uniform sampling configuration.
 *
 * Purpose:
 *   Translate the already resolved string values needed on every uniform event into a small immutable-
 *   by-convention value object, avoiding repeated map lookup and numeric parsing in the production loop.
 *
 * Workflow:
 *   RunConfig parses, resolves `auto`/`sampled`, and validates -> UniformConfig maps channel/mode strings
 *   and converts numeric fields once -> UniformGenerator reads the cached members while sampling events.
 *
 * Scope:
 *   This object caches particle-content and kinematic settings only. Output, formatting, RNG seeds, and
 *   fixed/target vertex controls remain in RunConfig because they are prepared or read outside the hot
 *   sampling path.
 */

#pragma once
#include "config/RunConfig.h"
#include "support/constants.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// UniformChannel object -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformChannel object */
/**
 * @enum UniformChannel
 * @brief Typed multiplicity selected from the validated 1e or eh setting.
 *
 * Purpose:
 *   Let the event loop dispatch particle construction without repeatedly comparing configuration text.
 *
 * Creation and use:
 *   UniformConfig maps one validated channel string to this enum. UniformGenerator then selects the
 *   corresponding multiplicity and sampling prescription for every event.
 *
 * Scientific meaning:
 *   Electron writes one sampled electron. ElectronHadron writes an artificial trigger electron followed
 *   by the separately selected proton, neutron, pi+, or pi-. Both are acceptance probes.
 */
enum class UniformChannel {
    Electron,        ///< `1e`: one generated electron.
    ElectronHadron,  ///< `eh`: trigger electron followed by the selected hadron.
};
#pragma endregion

// HadronSpecies object -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* HadronSpecies object */
/** @enum HadronSpecies @brief Typed identity selected by `--hadron` for the eh channel. */
enum class HadronSpecies { Proton, Neutron, PiPlus, PiMinus };
#pragma endregion

// Resolve string settings once, outside the production event loop.

// UniformConfig object --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformConfig object */
/**
 * @struct UniformConfig
 * @brief Typed, cached sampling settings used in the production loop.
 *
 * Purpose:
 *   Hold the complete set of typed values needed to choose uniform particle content and sample its
 *   kinematics without owning general run configuration or mutable generator state.
 *
 * Creation and lifetime:
 *   Construct once from a RunConfig returned by RunConfig::parse(..., true), after `auto` and `sampled`
 *   aliases are resolved and uniform constraints are validated. All values are copied; UniformConfig
 *   does not retain a reference and may safely outlive the source RunConfig.
 *
 * Units:
 *   Beam energy is GeV, momenta are GeV/c, and all angles are degrees. UniformGenerator converts angles
 *   to radians only at ROOT TVector3/math boundaries. A and Z are dimensionless LUND header metadata.
 *
 * Invariants:
 *   Exactly one channel enum is selected. Electron momentum is uniform-p, mixed p and 1/p, or beam-valued.
 *   Hadron momentum is exactly one of uniform-p, mixed p and 1/p, or fixed (both flags false). Hadron angle is
 *   uniform in cos(theta) when isotropic_hadron_angle is true and uniform in theta otherwise. Bounds,
 *   target metadata, and mode/channel compatibility were checked by RunConfig::validate(true).
 */
struct UniformConfig {
    // Channel and resolved prescriptions --------------------------------------------------------------------------------------------------------------------------------
    UniformChannel channel;          ///< Electron-only or electron-hadron branch.
    HadronSpecies hadron;            ///< Hadron identity used by the eh branch.
    int hadron_pid;                  ///< Centralized PDG identifier for the selected hadron.
    bool legacy_mass;                ///< Select archived rounded masses only when explicitly requested.
    bool uniform_electron_momentum;  ///< True: p~U(electron_p_min,electron_p_max).
    bool mixed_electron_momentum;    ///< True: 1e alternates uniform-p and uniform-1/p by event ID.
    bool uniform_hadron_momentum;    ///< True: p~U(p_min,p_max); mutually exclusive with mixed mode.
    bool mixed_hadron_momentum;      ///< True: charged hadrons alternate uniform-p and uniform-1/p by event ID.
    bool isotropic_hadron_angle;     ///< True: cos(theta) is uniform within the configured bounds.

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
     * @brief Cache one resolved and validated uniform configuration.
     *
     * @param c Borrowed RunConfig read only during construction. It must contain uniform-source keys,
     *          resolved automatic aliases, valid numeric text, and compatible bounds/modes.
     *
     * @throws std::exception If a required key is absent or a numeric conversion fails. This constructor
     *         deliberately does not call validate(), resolve aliases, or repair incompatible values.
     *
     * @note The channel fallback maps the only other validated value, `eh`, to ElectronHadron. Hadron text is likewise safe because RunConfig admits exactly four species.
     */
    explicit UniformConfig(const RunConfig& c)
        : channel(c.get("channel") == "1e" ? UniformChannel::Electron : UniformChannel::ElectronHadron),
          hadron(c.get("hadron") == "proton"    ? HadronSpecies::Proton
                 : c.get("hadron") == "neutron" ? HadronSpecies::Neutron
                 : c.get("hadron") == "pip"     ? HadronSpecies::PiPlus
                                                : HadronSpecies::PiMinus),
          hadron_pid(c.get("hadron") == "proton"    ? constants::proton_pdg
                     : c.get("hadron") == "neutron" ? constants::neutron_pdg
                     : c.get("hadron") == "pip"     ? constants::pi_plus_pdg
                                                    : constants::pi_minus_pdg),
          legacy_mass(c.get("mass-convention") == "legacy"),
          uniform_electron_momentum(c.get("electron-momentum") == "uniform"),
          mixed_electron_momentum(c.get("electron-momentum") == "mixed"),
          uniform_hadron_momentum(c.get("hadron-momentum") == "uniform"),
          mixed_hadron_momentum(c.get("hadron-momentum") == "mixed"),
          isotropic_hadron_angle(c.get("hadron-angle") == "isotropic"),
          beam(c.number("beam-energy")),
          electron_theta_min(c.number("electron-theta-min")),
          electron_theta_max(c.number("electron-theta-max")),
          electron_p_min(c.number("electron-p-min")),
          electron_p_max(c.number("electron-p-max")),
          hadron_theta_min(c.number("hadron-theta-min")),
          hadron_theta_max(c.number("hadron-theta-max")),
          hadron_p(c.number("hadron-p")),
          hadron_p_min(c.number("hadron-p-min")),
          hadron_p_max(c.number("hadron-p-max")),
          trigger_theta(c.number("trigger-theta")),
          trigger_phi_offset(c.number("trigger-phi-offset")),
          A(static_cast<int>(c.integer("A"))),
          Z(static_cast<int>(c.integer("Z"))) {}
};
#pragma endregion

#pragma endregion

}  // namespace samples
