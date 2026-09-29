/**
 * @file UniformConfig.h
 * @brief Converts checked text settings into values used by the uniform LUND creator.
 *
 * Purpose:
 *   RunConfig keeps settings as text so they can be recorded exactly in the run manifest. Repeated text
 *   comparisons and number conversions do not belong inside the event loop. UniformConfig converts those
 *   settings once before event creation starts.
 *
 * Execution flow:
 *   RunConfig checks and completes the text settings. The UniformConfig constructor copies them into
 *   numbers, true/false choices, and named channel and hadron values. The uniform LUND creator then reads
 *   those converted values while creating every event.
 *
 * Scope:
 *   UniformConfig stores only particle choices and particle-motion settings. Output paths, LUND text
 *   formatting, random seeds, and the target geometry remain in RunConfig.
 */

#pragma once

#include "core/config/RunConfig.h"
#include "core/lund/Event.h"

namespace samples {

// Uniform-event settings ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Uniform-event settings */

// Uniform event type ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Uniform event type */
/**
 * @enum UniformChannel
 * @brief Names the kind of random test event to create.
 *
 * Purpose:
 *   The `channel` setting begins as text such as `1e` or `eh`. Converting it once to UniformChannel lets
 *   the event loop choose the correct particles without comparing that text for every event.
 *
 * Creation and use:
 *   UniformConfig converts the checked text into one value below. The uniform LUND creator uses that value
 *   to choose the number of particles and how their motion is sampled.
 *
 * Meaning:
 *   Electron creates one electron with sampled momentum and angles. ElectronTester creates one electron at
 *   beam momentum while scanning its angles. ElectronHadron creates a trigger electron followed by the
 *   selected proton, neutron, positive pion, or negative pion. These are detector tests, not physical
 *   interaction models.
 */
enum class UniformChannel {
    Electron,        ///< `1e`: one electron with sampled momentum and angles.
    ElectronTester,  ///< `electron-tester`: one beam-momentum electron with sampled angles.
    ElectronHadron,  ///< `eh`: trigger electron followed by the selected hadron.
};
#pragma endregion

// Selected hadron -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Selected hadron */
/**
 * @enum HadronSpecies
 * @brief Names the second particle in an electron-hadron test event.
 *
 * Creation and use:
 *   UniformConfig converts the checked `hadron` setting into one value below. The uniform LUND creator
 *   uses it together with hadron_pid to create the particle written after the trigger electron.
 */
enum class HadronSpecies {
    Proton,   ///< Proton selected by `--hadron proton`.
    Neutron,  ///< Neutron selected by `--hadron neutron`.
    PiPlus,   ///< Positive pion selected by `--hadron pip`.
    PiMinus,  ///< Negative pion selected by `--hadron pim`.
};
#pragma endregion

// Converted uniform settings --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Converted uniform settings */
/**
 * @struct UniformConfig
 * @brief Stores particle choices and numeric ranges in forms ready for the event loop.
 *
 * Purpose:
 *   Keep the values needed to choose particles and randomly choose their momentum and angles. The object
 *   does not store the complete RunConfig and does not contain a changing random-number generator.
 *
 * Creation and lifetime:
 *   Create one UniformConfig after RunConfig has replaced `auto` and the older value `sampled`. Every
 *   needed value is copied, so the RunConfig does not need to remain alive for this object.
 *
 * Units:
 *   Beam energy is measured in GeV, momentum in GeV/c, and angles in degrees. The uniform LUND creator
 *   converts angles to radians only when calling ROOT vector or math functions. A is the total number of
 *   protons and neutrons written in the LUND header. Z is the number of protons.
 *
 * Rules:
 *   channel contains exactly one event type. Electron momentum is sampled uniformly in p, sampled with
 *   the mixed p and 1/p method, or fixed at beam momentum. Hadron momentum is uniform, mixed, or fixed;
 *   both hadron flags are false in fixed mode. Hadron theta and phi are always sampled uniformly inside
 *   their configured ranges. RunConfig has already checked that all choices work together.
 */
struct UniformConfig {
    // Particle and sampling choices -------------------------------------------------------------------------------------------------------------------------------------
    UniformChannel channel;          ///< Kind of test event to create.
    HadronSpecies hadron;            ///< Hadron used when channel is ElectronHadron.
    int hadron_pid;                  ///< Standard PDG integer for the selected hadron.
    bool uniform_electron_momentum;  ///< true to choose electron momentum uniformly between its limits.
    bool mixed_electron_momentum;    ///< true to alternate uniform-p and uniform-1/p by event ID.
    bool uniform_hadron_momentum;    ///< true to choose hadron momentum uniformly between its limits.
    bool mixed_hadron_momentum;      ///< true to alternate uniform-p and uniform-1/p by event ID.

    // Numeric ranges and target values ----------------------------------------------------------------------------------------------------------------------------------
    double beam;                ///< Beam energy in GeV; beam-momentum particles use the same numeric value.
    double electron_theta_min;  ///< Lower electron angle from the beam direction, in degrees.
    double electron_theta_max;  ///< Upper electron polar-angle bound in degrees.
    double electron_p_min;      ///< Lower electron-momentum limit in GeV/c.
    double electron_p_max;      ///< Upper electron-momentum limit in GeV/c.
    double hadron_theta_min;    ///< Lower hadron angle from the beam direction, in degrees.
    double hadron_theta_max;    ///< Upper hadron angle from the beam direction, in degrees.
    double hadron_p;            ///< Neutron momentum in GeV/c when fixed mode is selected.
    double hadron_p_min;        ///< Lower hadron-momentum limit in GeV/c.
    double hadron_p_max;        ///< Upper hadron-momentum limit in GeV/c.
    double trigger_theta;       ///< Trigger-electron angle from the beam direction, in degrees.
    double trigger_phi_offset;  ///< Angle away from the chosen opposite-sector center, in degrees.
    int A;                      ///< Total number of protons and neutrons written in the LUND header.
    int Z;                      ///< Number of protons written in the LUND header.

    /**
     * @brief Constructor: Copy and convert one checked set of uniform-event settings.
     * @param c Checked uniform LUND creator settings with every automatic value already replaced.
     * @throws std::exception If a required setting is missing or its number cannot be converted. This
     *                        constructor assumes RunConfig has already checked the setting combinations.
     * @note Any channel text other than `1e` or `electron-tester` becomes ElectronHadron. Any hadron text
     *       other than `proton`, `neutron`, or `pip` becomes PiMinus. This is safe only because
     *       RunConfig rejects all other values first.
     */
    explicit UniformConfig(const RunConfig& c)
        : channel((c.getText("channel") == "1e")                ? UniformChannel::Electron
                  : (c.getText("channel") == "electron-tester") ? UniformChannel::ElectronTester
                                                                : UniformChannel::ElectronHadron),
          hadron((c.getText("hadron") == "proton")    ? HadronSpecies::Proton
                 : (c.getText("hadron") == "neutron") ? HadronSpecies::Neutron
                 : (c.getText("hadron") == "pip")     ? HadronSpecies::PiPlus
                                                      : HadronSpecies::PiMinus),
          hadron_pid((c.getText("hadron") == "proton")    ? constants::proton_pdg
                     : (c.getText("hadron") == "neutron") ? constants::neutron_pdg
                     : (c.getText("hadron") == "pip")     ? constants::pi_plus_pdg
                                                          : constants::pi_minus_pdg),
          uniform_electron_momentum((c.getText("electron-momentum") == "uniform")),
          mixed_electron_momentum((c.getText("electron-momentum") == "mixed")),
          uniform_hadron_momentum((c.getText("hadron-momentum") == "uniform")),
          mixed_hadron_momentum((c.getText("hadron-momentum") == "mixed")),
          beam(c.getDouble("beam-energy")),
          electron_theta_min(c.getDouble("electron-theta-min")),
          electron_theta_max(c.getDouble("electron-theta-max")),
          electron_p_min(c.getDouble("electron-p-min")),
          electron_p_max(c.getDouble("electron-p-max")),
          hadron_theta_min(c.getDouble("hadron-theta-min")),
          hadron_theta_max(c.getDouble("hadron-theta-max")),
          hadron_p(c.getDouble("hadron-p")),
          hadron_p_min(c.getDouble("hadron-p-min")),
          hadron_p_max(c.getDouble("beam-energy")),
          trigger_theta(c.getDouble("trigger-theta")),
          trigger_phi_offset(c.getDouble("trigger-phi-offset")),
          A(static_cast<int>(c.getNonnegativeInteger("A"))),
          Z(static_cast<int>(c.getNonnegativeInteger("Z"))) {}
};
#pragma endregion

#pragma endregion

}  // namespace samples
