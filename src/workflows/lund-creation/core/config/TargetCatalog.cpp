//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetCatalog.cpp
 * @brief Implements target lookup and beam-dependent GEMC variation selection.
 *
 * Purpose:
 *   Translate one material identity plus beam energy into coherent LUND metadata, GEMC target variation,
 *   and targets.h vertex geometry.
 *
 * Workflow:
 *   Look up the target -> honor a checked explicit variation when present -> otherwise apply the standard
 *   RG-M target/beam mapping -> return the matching geometry record.
 *
 * Selection rules:
 *   H1, D2, He4, Ar40, calcium, and tin have one supported variation. C12 uses the small foil at
 *   2.07052 GeV, the large foil at 4.02962 GeV, and four foils at 5.98636 GeV. An explicit compatible
 *   variation handles exceptions such as run 15733, which used the small C12 foil at 4.02962 GeV.
 *
 * Failure:
 *   Unknown targets and variations, incompatible target/variation pairs, and C12 energies without an
 *   automatic mapping are rejected before output creation.
 */

#include "core/config/TargetCatalog.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace samples {

namespace {

// Variation records -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Variation records */
const std::vector<TargetVariation>& targetVariations() {
    static const std::vector<TargetVariation> variations = {
        // Liquid-target variations share the external liquid-cell geometry.
        {"rga_spring2019", "H1", "liquid"},
        {"rgb_fall2019", "D2", "liquid"},
        {"rgm_fall2021_He", "He4", "liquid"},

        // Carbon uses one detector assembly for each nominal beam energy.
        {"rgm_fall2021_C_S", "C12", "1-foil-small"},
        {"rgm_fall2021_C_L", "C12", "1-foil-large"},
        {"rgm_fall2021_Cx4", "C12", "4-foil"},

        // The remaining solid targets each have one supported detector assembly.
        {"rgm_fall2021_Snx4", "Sn-nat", "4-foil"},
        {"rgm_fall2021_Ca", "Ca40", "Ca"},
        {"rgm_fall2021_Ca", "Ca48", "Ca"},
        {"rgm_fall2021_Ar", "Ar40", "Ar"},
        {"rgm_fall2021_Sn_L", "Sn120", "1-foil-large"},
    };

    return variations;
}

const TargetVariation& findVariation(const std::string& target, const std::string& identifier) {
    for (const auto& variation : targetVariations()) {
        if (variation.target == target && variation.identifier == identifier) { return variation; }
    }

    throw std::runtime_error("GEMC target variation '" + identifier + "' is unknown or incompatible with target '" + target + "'");
}
#pragma endregion

}  // namespace

// Target records --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target records */
const std::vector<Target>& targets() {
    static const std::vector<Target> catalog = {
        {"H1", "hydrogen-1", 1, 1},     {"D2", "deuterium", 2, 1},      {"He4", "helium-4", 4, 2},    {"C12", "carbon-12", 12, 6},   {"Sn-nat", "natural tin", 119, 50},
        {"Ca40", "calcium-40", 40, 20}, {"Ca48", "calcium-48", 48, 20}, {"Ar40", "argon-40", 40, 18}, {"Sn120", "tin-120", 120, 50},
    };

    return catalog;
}
#pragma endregion

// Target lookup ---------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target lookup */
const Target& findTarget(const std::string& identifier) {
    for (const auto& target : targets()) {
        if (target.identifier == identifier) { return target; }
    }

    throw std::runtime_error("Unknown target '" + identifier + "'; choose one of: " + targetNames());
}
#pragma endregion

// Variation resolution --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Variation resolution */
const TargetVariation& resolveTargetVariation(const Target& target, double beam_energy, const std::string& override_name) {
    if (override_name != "auto") { return findVariation(target.identifier, override_name); }

    if (target.identifier == "C12") {
        if (std::abs(beam_energy - 2.07052) < 1e-6) { return findVariation(target.identifier, "rgm_fall2021_C_S"); }
        if (std::abs(beam_energy - 4.02962) < 1e-6) { return findVariation(target.identifier, "rgm_fall2021_C_L"); }
        if (std::abs(beam_energy - 5.98636) < 1e-6) { return findVariation(target.identifier, "rgm_fall2021_Cx4"); }

        throw std::runtime_error("No automatic GEMC target variation for C12 at beam energy " + std::to_string(beam_energy) + " GeV; specify --gemc-target-variation");
    }

    for (const auto& variation : targetVariations()) {
        if (variation.target == target.identifier) { return variation; }
    }

    throw std::runtime_error("No GEMC target variation is configured for target '" + target.identifier + "'");
}
#pragma endregion

// Target-name presentation ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target-name presentation */
std::string targetNames() {
    std::ostringstream out;
    bool first = true;

    for (const auto& target : targets()) {
        out << (first ? "" : ", ") << target.identifier;
        first = false;
    }

    return out.str();
}
#pragma endregion

}  // namespace samples
