//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RgmTarget.cpp
 * @brief Stores the supported RG-M target records.
 *
 * Purpose:
 *   Keep the default A, Z, targets.h geometry name, and GEMC variation for each RG-M target in one list.
 *
 * Workflow:
 *   Create the list on first use -> return read-only references -> look up exact target names -> use the
 *   same ordered names in help and error messages.
 *
 * Scope:
 *   This file stores names and defaults only. Other files sample vertices and configure GEMC. RG-M
 *   production uses Ar40 at every beam energy, small one-foil C12 at 2 GeV, large one-foil C12 at 4 GeV,
 *   and four-foil C12 at 6 GeV. Run 15733 is an exception that used small one-foil C12 at 4 GeV.
 */

#include "core/config/RgmTarget.h"

#include <sstream>
#include <stdexcept>

namespace samples {

// RG-M catalog ----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Catalog */
const std::vector<RgmTarget>& rgmTargets() {
    // This order is used in help and error messages. Beam-energy guidance is documented separately;
    // lookup still requires only an exact target name so explicit studies remain possible.
    static const std::vector<RgmTarget> targets = {
        {"H1", "liquid hydrogen", 1, 1, "liquid", "rga_spring2019"},
        {"D2", "liquid deuterium", 2, 1, "liquid", "rgb_fall2019"},
        {"He4", "liquid helium-4", 4, 2, "liquid", "rgm_fall2021_He"},
        {"C12-four-foil", "carbon-12 four-foil", 12, 6, "4-foil", "rgm_fall2021_Cx4"},
        {"Sn-nat-four-foil", "natural-tin four-foil", 119, 50, "4-foil", "rgm_fall2021_Snx4"},
        {"Ca40", "calcium-40 single foil", 40, 20, "Ca", "rgm_fall2021_Ca"},
        {"Ca48", "calcium-48 single foil", 48, 20, "Ca", "rgm_fall2021_Ca"},
        {"C12-small", "carbon-12 small 4 mm single foil", 12, 6, "1-foil-small", "rgm_fall2021_C_S"},
        {"C12-large", "carbon-12 large 6 mm single foil", 12, 6, "1-foil-large", "rgm_fall2021_C_L"},
        {"Ar40", "liquid argon-40 short cryocell", 40, 18, "Ar", "rgm_fall2021_Ar"},
        {"Sn120-large", "tin-120 large 6 mm single foil", 120, 50, "1-foil-large", "rgm_fall2021_Sn_L"},
    };

    // Return the stored list without copying it.
    return targets;
}
#pragma endregion

// Exact target lookup ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Exact target lookup */
const RgmTarget& findRgmTarget(const std::string& identifier) {
    // A direct loop is clear enough for this small list.
    for (const auto& target : rgmTargets()) {
        if (target.identifier == identifier) { return target; }
    }

    throw std::runtime_error("Unknown RG-M target '" + identifier + "'; choose one of: " + rgmTargetNames());
}
#pragma endregion

// Target-name presentation ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target-name presentation */
std::string rgmTargetNames() {
    std::ostringstream out;
    bool first = true;

    for (const auto& target : rgmTargets()) {
        out << (first ? "" : ", ") << target.identifier;
        first = false;
    }

    return out.str();
}
#pragma endregion

}  // namespace samples
