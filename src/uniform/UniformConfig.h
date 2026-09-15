//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file UniformConfig.h
 * @brief Typed uniform sampling configuration.
 *
 * Purpose:
 *   Resolve string-valued settings once outside the event loop.
 *
 * Workflow:
 *   RunConfig resolves and validates options; UniformConfig caches numeric values and mode flags.
 */

#pragma once
#include "common/RunConfig.h"
namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// UniformChannel object ------------------------------------------------
#pragma region /* UniformChannel object */
/**
 * @enum UniformChannel
 * @brief Typed channel selected from the validated 1e, ep or en setting.
 *
 * Purpose: let the event loop dispatch without repeatedly interpreting strings.
 * Usage: UniformConfig selects the value once; UniformGenerator chooses the particle prescription.
 * Values: Electron produces an electron; ElectronProton and ElectronNeutron add the named
 * nucleon to an artificial trigger electron. These are acceptance probes, not exclusive reactions.
 */
enum class UniformChannel { Electron, ElectronProton, ElectronNeutron };
#pragma endregion
// Resolve string settings once, outside the production event loop.
// UniformConfig object ------------------------------------------------
#pragma region /* UniformConfig object */
/**
 * @struct UniformConfig
 * @brief Typed, cached sampling settings used in the production loop.
 *
 * Construct only after RunConfig has resolved auto/sampled modes and validated bounds.
 * Angles remain in degrees here; the generator converts to radians when constructing vectors.
 * Momentum and beam energy are in GeV. A/Z do not select the vertex geometry.
 */
struct UniformConfig {
    // Channel and resolved prescriptions ---------------------------------------
    UniformChannel channel;  ///< Select the particle content of each event.
    // The flags encode resolved modes: uniform-p, alternating p/1/p, or fixed momentum;
    // isotropic angles mean uniform cos(theta) inside configured angular bounds.
    bool uniform_electron_momentum, uniform_nucleon_momentum, mixed_nucleon_momentum, isotropic_nucleon_angle;

    // Physical ranges and trigger settings -------------------------------------
    double beam, electron_theta_min, electron_theta_max;  ///< GeV beam scale and degree limits.
    // Nucleon angles are in degrees; fixed momentum and lower/upper bounds are in GeV.
    double nucleon_theta_min, nucleon_theta_max, nucleon_p, nucleon_p_min, nucleon_p_max;
    double trigger_theta, trigger_phi_offset;  ///< Artificial trigger angles in degrees.
    int A, Z;  ///< LUND target metadata; independent of the vertex geometry.
    /** @brief Cache a validated configuration; this constructor does not resolve auto values. */
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
