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
#include "common/RunConfig.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// UniformChannel object -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* UniformChannel object */
/**
 * @enum UniformChannel
 * @brief Typed channel selected from the validated 1e, ep or en setting.
 *
 * Purpose:
 *   Let the event loop dispatch particle construction without repeatedly comparing configuration text.
 *
 * Creation and use:
 *   UniformConfig maps one validated channel string to this enum. UniformGenerator then selects the
 *   corresponding multiplicity, nucleon identity, and sampling prescription for every event.
 *
 * Scientific meaning:
 *   Electron writes one sampled electron. ElectronProton and ElectronNeutron write an artificial
 *   trigger electron followed by the named nucleon. They are unphysical acceptance probes rather than
 *   exclusive interaction channels.
 */
enum class UniformChannel {
    Electron,         ///< `1e`: one generated electron.
    ElectronProton,   ///< `ep`: trigger electron followed by a proton.
    ElectronNeutron,  ///< `en`: trigger electron followed by a neutron.
};
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
 *   Exactly one channel enum is selected. Electron momentum is either uniform or beam-valued. Nucleon
 *   momentum is exactly one of uniform-p, mixed p and 1/p, or fixed (both flags false). Nucleon angle is
 *   uniform in cos(theta) when isotropic_nucleon_angle is true and uniform in theta otherwise. Bounds,
 *   target metadata, and mode/channel compatibility were checked by RunConfig::validate(true).
 */
struct UniformConfig {
    // Channel and resolved prescriptions --------------------------------------------------------------------------------------------------------------------------------
    UniformChannel channel;          ///< Particle content and electron/nucleon branch.
    bool uniform_electron_momentum;  ///< True: p~U(0,Ebeam); false: p uses the beam value under c=1.
    bool uniform_nucleon_momentum;   ///< True: p~U(p_min,p_max); mutually exclusive with mixed mode.
    bool mixed_nucleon_momentum;     ///< True: ep alternates uniform-p and uniform-1/p by event ID.
    bool isotropic_nucleon_angle;    ///< True: cos(theta) is uniform within the configured bounds.

    // Physical ranges and trigger settings ------------------------------------------------------------------------------------------------------------------------------
    double beam;                ///< Beam energy in GeV; also supplies the c=1 momentum-scale value.
    double electron_theta_min;  ///< Inclusive lower electron polar-angle bound in degrees.
    double electron_theta_max;  ///< Upper electron polar-angle bound in degrees.
    double nucleon_theta_min;   ///< Inclusive lower nucleon polar-angle bound in degrees.
    double nucleon_theta_max;   ///< Upper nucleon polar-angle bound in degrees.
    double nucleon_p;           ///< Fixed-mode nucleon momentum in GeV/c.
    double nucleon_p_min;       ///< Lower sampled-momentum bound in GeV/c.
    double nucleon_p_max;       ///< Upper sampled-momentum bound in GeV/c.
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
     * @note The final channel fallback maps any value other than `1e` or `ep` to ElectronNeutron; this
     *       is safe only because the required RunConfig::validate(true) contract admits `en` alone.
     */
    explicit UniformConfig(const RunConfig& c)
        : channel(c.get("channel") == "1e"   ? UniformChannel::Electron
                  : c.get("channel") == "ep" ? UniformChannel::ElectronProton
                                             : UniformChannel::ElectronNeutron),
          uniform_electron_momentum(c.get("electron-momentum") == "uniform"),
          uniform_nucleon_momentum(c.get("nucleon-momentum") == "uniform"),
          mixed_nucleon_momentum(c.get("nucleon-momentum") == "mixed"),
          isotropic_nucleon_angle(c.get("nucleon-angle") == "isotropic"),
          beam(c.number("beam-energy")),
          electron_theta_min(c.number("electron-theta-min")),
          electron_theta_max(c.number("electron-theta-max")),
          nucleon_theta_min(c.number("nucleon-theta-min")),
          nucleon_theta_max(c.number("nucleon-theta-max")),
          nucleon_p(c.number("nucleon-p")),
          nucleon_p_min(c.number("nucleon-p-min")),
          nucleon_p_max(c.number("nucleon-p-max")),
          trigger_theta(c.number("trigger-theta")),
          trigger_phi_offset(c.number("trigger-phi-offset")),
          A(static_cast<int>(c.integer("A"))),
          Z(static_cast<int>(c.integer("Z"))) {}
};
#pragma endregion

#pragma endregion

}  // namespace samples
