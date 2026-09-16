/**
 * @file RgmTarget.cpp
 * @brief RG-M catalog mapped onto the replaceable external geometry implementation.
 *
 * Workflow: immutable table -> exact identifier lookup -> configuration resolution.
 */
#include "common/RgmTarget.h"
#include <sstream>
#include <stdexcept>
namespace samples {
#pragma region /* Catalog */
const std::vector<RgmTarget>& rgmTargets() {
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
        {"C12-legacy", "archived carbon-12 single foil", 12, 6, "1-foil", "rgm_fall2021_C"},
        {"Sn120-legacy", "archived tin-120 single foil", 120, 50, "1-foil", "rgm_fall2021_Sn"},
    };
    return targets;
}
#pragma endregion
#pragma region /* Lookup */
const RgmTarget& findRgmTarget(const std::string& identifier) {
    for (const auto& target : rgmTargets()) if (target.identifier == identifier) return target;
    throw std::runtime_error("Unknown RG-M target '" + identifier + "'; choose one of: " + rgmTargetNames());
}
std::string rgmTargetNames() {
    std::ostringstream out;
    bool first = true;
    for (const auto& target : rgmTargets()) { out << (first ? "" : ", ") << target.identifier; first = false; }
    return out.str();
}
#pragma endregion
}  // namespace samples
