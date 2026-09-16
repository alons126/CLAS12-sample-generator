/** @file RgmTarget.h @brief Maintained RG-M target identity and geometry catalog. */
#pragma once
#include <string>
#include <vector>
namespace samples {
#pragma region /* RG-M target record */
/**
 * @struct RgmTarget
 * @brief Material and assembly metadata for one supported RG-M target.
 *
 * A/Z populate LUND; geometry selects protected targets.h; gemc_variation selects detector geometry.
 */
struct RgmTarget {
    std::string identifier, description;
    int A, Z;
    std::string geometry, gemc_variation;
};
#pragma endregion
const std::vector<RgmTarget>& rgmTargets();
const RgmTarget& findRgmTarget(const std::string& identifier);
std::string rgmTargetNames();
}  // namespace samples
