//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RgmTarget.cpp
 * @brief RG-M catalog mapped onto the replaceable external geometry implementation.
 *
 * Purpose:
 *   Maintain the small, reviewable mapping from RG-M target identity to the default A/Z header values,
 *   protected targets.h geometry key, and GEMC target-variation provenance used by RunConfig.
 *
 * Workflow:
 *   Lazily initialize immutable catalog storage -> expose it by const reference -> perform exact
 *   identifier lookup during configuration resolution -> format the same ordered identifiers for help
 *   and invalid-target diagnostics.
 *
 * Separation of responsibilities:
 *   This file owns target-identity metadata only. TargetGeometry and protected targets.h own spatial
 *   vertex distributions; detector GCARD resources own GEMC geometry implementation.
 */

#include "common/RgmTarget.h"

#include <sstream>
#include <stdexcept>

namespace samples {

// RG-M catalog ---------------------------------------------------------------

#pragma region /* Catalog */
/**
 * @brief Return the immutable catalog of supported RG-M target identities.
 *
 * Purpose:
 *   Keep automatic target defaults in one source-controlled table instead of scattering A/Z, external
 *   geometry keys, and GEMC variation labels across profiles and workflow code.
 *
 * Construction:
 *   C++ initializes the function-local static vector once on first use. Aggregate fields follow the
 *   RgmTarget order: identifier, description, A, Z, geometry, and gemc_variation.
 *
 * @return Const reference to process-lifetime catalog storage in maintained display order.
 *
 * @note Shared geometry keys are deliberate. For example, isotopes may use the same spatial assembly
 *       while retaining different nuclear metadata, and several identities may use one foil shape with
 *       distinct GEMC variations. RunConfig applies each field independently only when set to `auto`.
 */
const std::vector<RgmTarget>& rgmTargets() {
    // Catalog order controls help/error presentation only; lookup is by exact identifier. Records are
    // immutable after this thread-safe first initialization.
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

    // Returning by const reference avoids rebuilding or copying the catalog and keeps references from
    // findRgmTarget() valid for the remainder of the process.
    return targets;
}
#pragma endregion

// Exact target lookup --------------------------------------------------------

#pragma region /* Exact target lookup */
/**
 * @brief Resolve one case-sensitive RG-M identifier to its catalog record.
 *
 * Algorithm:
 *   Scan the small maintained table in order and return the first exact identifier match. If no record
 *   matches, build one diagnostic containing the supplied value and every supported identifier.
 *
 * @param identifier User/profile value of `rgm-target`; no trimming or case conversion occurs here.
 *
 * @return Const reference into the process-lifetime vector returned by rgmTargets().
 *
 * @throws std::runtime_error If identifier is unknown. No fallback target is selected silently.
 */
const RgmTarget& findRgmTarget(const std::string& identifier) {
    // Linear lookup keeps the catalog representation direct and readable; its fixed small size does not
    // justify a second index whose contents or ordering could diverge.
    for (const auto& target : rgmTargets())
        if (target.identifier == identifier) return target;

    throw std::runtime_error("Unknown RG-M target '" + identifier + "'; choose one of: " + rgmTargetNames());
}
#pragma endregion

// Target-name presentation --------------------------------------------------

#pragma region /* Target-name presentation */
/**
 * @brief Join catalog identifiers for CLI help and lookup failures.
 *
 * Algorithm:
 *   Traverse rgmTargets() in maintained order, write a separator before every item except the first,
 *   and return the accumulated text.
 *
 * @return Newly owned comma-and-space-separated list with no leading or trailing separator.
 *
 * @note Presentation only; this function neither validates a selection nor exposes descriptions and
 *       geometry metadata.
 */
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
