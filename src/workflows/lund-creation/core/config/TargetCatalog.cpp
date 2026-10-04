/**
 * @file TargetCatalog.cpp
 * @brief Stores the supported targets and chooses the matching GEMC target setup.
 *
 * Purpose:
 *   Turn a target name and beam energy into the settings needed by later workflow stages. For example,
 *   target `C12` at 2.07052 GeV selects the small carbon foil in GEMC and the matching vertex-position
 *   rules in the external targets.h.
 *
 * Execution flow:
 *   findTarget() finds the requested material and its default A and Z values. resolveTargetVariation()
 *   uses a setup named by the user when one is given. Otherwise, it selects the usual GEMC target setup
 *   from the target name and beam energy. The selected record also gives the vertex-position rules used by
 *   targets.h.
 *
 * Selection rules:
 *   H1, D2, He4, Ar40, calcium, and tin each have one supported setup. C12 uses the small foil at 2.07052
 *   GeV, the large foil at 4.02962 GeV, and four foils at 5.98636 GeV. The user can name a compatible
 *   setup directly for an exception. For example, run 15733 used the small C12 foil at 4.02962 GeV.
 *
 * Failure:
 *   The program stops before creating output when the target or setup name is unknown, the setup does not
 *   belong to the selected target, or no usual C12 setup exists for the beam energy.
 */

#include "core/config/TargetCatalog.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace samples {

/**
 * @namespace samples::<anonymous>
 * @brief Keeps target-variation records and lookup helpers private to this source file.
 *
 * Purpose:
 *   Store the supported GEMC target setups and find one that belongs to a requested target. These details
 *   support the public target-selection functions but are not part of the interface in TargetCatalog.h.
 */
namespace {

// Available GEMC target setups ------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Available GEMC target setups */
/**
 * @brief Get every supported GEMC target setup.
 * @return The setup list. The caller may read it but not change it, and it remains available until the
 *         program ends.
 */
const std::vector<TargetVariation>& targetVariations() {
    // Used RG-M targets and variations. For more details, see:
    //  1. RG-M analysis note.
    //  2. CLAS12 Note 2026-001: https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185
    static const std::vector<TargetVariation> variations = {
        // Three liquid targets use the same vertex-position rules for the 5-cm-long cryocell.
        {"rga_spring2019", "H1", "liquid"},
        {"rgb_fall2019", "D2", "liquid"},
        {"rgm_fall2021_He", "He4", "liquid"},

        // Liquid Ar target uses the vertex-position rules for the 0.5-cm-long cryocell.
        {"rgm_fall2021_Ar", "Ar40", "Ar"},

        // Each carbon setup describes a different number or size of foils.
        {"rgm_fall2021_C_S", "C12", "1-foil-small"},  // 4-mm wide foils
        {"rgm_fall2021_C_L", "C12", "1-foil-large"},  // 6-mm wide foils
        {"rgm_fall2021_Cx4", "C12", "4-foil"},

        // Each remaining solid target has one supported GEMC setup.
        {"rgm_fall2021_Ca", "Ca40", "Ca"},
        {"rgm_fall2021_Ca", "Ca48", "Ca"},
        {"rgm_fall2021_Sn_L", "Sn120", "1-foil-large"},  // 6-mm wide foils
        {"rgm_fall2021_Snx4", "Sn-nat", "4-foil"},
    };

    return variations;
}

/**
 * @brief Find one GEMC target setup that belongs to the selected target.
 * @param target Exact target name, such as `C12`.
 * @param identifier Exact GEMC setup name, such as `rgm_fall2021_C_S`.
 * @return The matching setup. The caller may read it but not change it.
 * @throws std::runtime_error If the setup name is unknown or belongs to a different target.
 */
const TargetVariation& findVariation(const std::string& target, const std::string& identifier) {
    for (const auto& variation : targetVariations()) {
        if ((variation.target == target) && (variation.identifier == identifier)) { return variation; }
    }

    // The loop checked every stored setup and found none with both requested names. The setup therefore
    // either does not exist or belongs to a different target, so the program cannot safely use it.
    throw std::runtime_error("GEMC target variation '" + identifier + "' is unknown or incompatible with target '" + target + "'");
}
#pragma endregion

}  // namespace

// Supported target materials --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Supported target materials */
const std::vector<Target>& targets() {
    static const std::vector<Target> catalog = {
        // Targets that use the 5-cm-long liquid-cell geometry.
        {"H1", "hydrogen-1", 1, 1},
        {"D2", "deuterium", 2, 1},
        {"He4", "helium-4", 4, 2},

        // Liquid argon uses its own 0.5-cm-long cell geometry.
        {"Ar40", "liquid-argon-40", 40, 18},

        // Carbon target.
        {"C12", "carbon-12", 12, 6},

        // Calcium targets.
        {"Ca40", "calcium-40", 40, 20},
        {"Ca48", "calcium-48", 48, 20},

        // Tin targets.
        {"Sn120", "tin-120", 120, 50},
        {"Sn-nat", "natural tin", 119, 50},
    };

    // Return access to this stored list instead of making a copy of the whole list.
    return catalog;
}
#pragma endregion

// Target name lookup ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target name lookup */
const Target& findTarget(const std::string& identifier) {
    for (const auto& target : targets()) {
        if (target.identifier == identifier) { return target; }
    }

    // No supported target has this exact name. Stop and include the valid names in the error so the user
    // can correct the requested value.
    throw std::runtime_error("Unknown target '" + identifier + "'; choose one of: " + targetNames());
}
#pragma endregion

// GEMC target setup selection -------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* GEMC target setup selection */
const TargetVariation& resolveTargetVariation(const Target& target, double beam_energy, const std::string& override_name) {
    // `auto` means that the program should choose a GEMC target setup below. Any other value is a setup
    // name supplied by the user. In that case, findVariation() checks that the name exists and belongs to
    // the selected target, then returns its stored settings.
    if (override_name != "auto") { return findVariation(target.identifier, override_name); }

    // For C12, find the used foil setup according to the beam energy.
    if (target.identifier == "C12") {
        if (std::abs(beam_energy - 2.07052) < 1e-6) { return findVariation(target.identifier, "rgm_fall2021_C_S"); }
        if (std::abs(beam_energy - 4.02962) < 1e-6) { return findVariation(target.identifier, "rgm_fall2021_C_L"); }
        if (std::abs(beam_energy - 5.98636) < 1e-6) { return findVariation(target.identifier, "rgm_fall2021_Cx4"); }

        // No usual C12 setup is known for any other energy. Stop and require the user to name the correct
        // setup instead of silently choosing one that may describe the wrong physical target.
        throw std::runtime_error("No automatic GEMC target variation for C12 at beam energy " + std::to_string(beam_energy) + " GeV; specify --gemc-target-variation");
    }

    // Every supported target other than C12 has one usual GEMC setup, so its beam energy does not affect
    // this choice. Check each stored setup until its target name matches the selected target.
    for (const auto& variation : targetVariations()) {
        if (variation.target == target.identifier) { return variation; }
    }

    // The target is supported, but the setup list contains no entry for it. Stop instead of continuing
    // without the GEMC setup and vertex-position rules needed by later stages.
    throw std::runtime_error("No GEMC target variation is configured for target '" + target.identifier + "'");
}
#pragma endregion

// Target names for messages ---------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target names for messages */
std::string targetNames() {
    // Build the result one piece at a time. `first` remembers whether a target name has already been added.
    std::ostringstream out;
    bool first = true;

    // Visit every supported target without copying or changing it. Add no separator before the first name;
    // add `, ` before every later name. Then append the target's identifier and remember that the first
    // name has been written.
    for (const auto& target : targets()) {
        out << (first ? "" : ", ") << target.identifier;
        first = false;
    }

    // Convert the completed text into the std::string returned to the caller.
    return out.str();
}
#pragma endregion

}  // namespace samples
