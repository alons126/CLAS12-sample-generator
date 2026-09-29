//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetCatalog.h
 * @brief Lists supported target materials and selects the settings used to simulate them.
 *
 * Purpose:
 *   Let the user select a target such as `C12` or `Ar40`. Each target supplies the default A and Z values
 *   written in the LUND event header.
 *
 *   The selected target and beam energy also choose the usual GEMC target variation and vertex geometry.
 *   The GEMC target variation names the target setup used by detector simulation. The vertex geometry
 *   tells the external targets.h where inside that target to place each event. The user may select a
 *   different, compatible GEMC target variation when needed.
 *
 * Workflow:
 *   RunConfig reads the requested target and beam energy. findTarget() checks that the target is supported.
 *   resolveTargetVariation() selects the usual GEMC target variation or checks the one supplied by the
 *   user. The selected variation provides the vertex-geometry name used by targets.h. User-supplied A or
 *   Z values replace the target defaults after these checks.
 *
 * Scope:
 *   This interface only looks up target information and selects setting names. It does not choose an
 *   event position inside the target, load a GEMC configuration file, or guess the target from A and Z.
 */

#pragma once

#include <string>
#include <vector>

namespace samples {

// Supported targets and settings ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Supported targets and settings */

// Target materials ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target materials */
/**
 * @struct Target
 * @brief Stores the name and default A and Z values for one target material.
 *
 * `identifier` is the value accepted by `--target`. A is the total number of protons and neutrons in
 * the target nucleus. Z is its number of protons. The program writes these values in each LUND event
 * header unless the user replaces them with `--A` or `--Z`.
 */
struct Target {
    std::string identifier;   ///< Exact name accepted by `--target`, such as `C12` or `Ar40`.
    std::string description;  ///< Readable material name, such as `carbon-12`.
    int A;                    ///< Default total number of protons and neutrons.
    int Z;                    ///< Default number of protons.
};
#pragma endregion

// Simulation settings ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Simulation settings */
/**
 * @struct TargetVariation
 * @brief Connects one target material to its GEMC setup and event-position rules.
 *
 * `identifier` names the target setup used by GEMC detector simulation. The program records this name in
 * the run log manifest and later gives it to the simulation-submission workflow. `target` states which
 * material may use that setup. `geometry` tells the external targets.h code where it may place an event
 * inside the target.
 */
struct TargetVariation {
    std::string identifier;  ///< Name of the target setup used by GEMC.
    std::string target;      ///< Target material that may use this setup.
    std::string geometry;    ///< Name of the event-position rules in targets.h.
};
#pragma endregion

// Target lookups --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target lookups */
/**
 * @brief Get all supported targets in the order used by help and error messages.
 * @return The target list. The caller may read it but not change it, and it remains available until the
 *         program ends.
 */
const std::vector<Target>& targets();

/**
 * @brief Find the settings for one target name.
 * @param identifier Exact target name, including capitalization, such as `H1`, `C12`, or `Ar40`.
 * @return The matching target settings. The caller may read them but not change them.
 * @throws std::runtime_error If the name is not in the supported target list.
 */
const Target& findTarget(const std::string& identifier);

/**
 * @brief Choose the GEMC target setup and event-position rules for a target.
 * @param target Target returned by findTarget().
 * @param beam_energy Beam energy in GeV.
 * @param override_name Use `auto` to select the usual setup for the target and beam energy. Otherwise,
 *                      give an exact compatible setup name such as `rgm_fall2021_C_S`.
 * @return The selected GEMC setup and event-position rules. The caller may read them but not change them.
 * @throws std::runtime_error If `auto` cannot select a setup, the given setup name is unknown, or that
 *                            setup cannot be used with the selected target.
 */
const TargetVariation& resolveTargetVariation(const Target& target, double beam_energy, const std::string& override_name);

/**
 * @brief Build the list of supported target names shown in help and error messages.
 * @return Target names separated by commas, in the same order as targets().
 */
std::string targetNames();
#pragma endregion

#pragma endregion

}  // namespace samples
