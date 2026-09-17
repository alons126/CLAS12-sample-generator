//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RgmTarget.h
 * @brief Maintained RG-M target identity and default-metadata catalog.
 *
 * Purpose:
 *   Map one user-facing RG-M target identifier to the nuclear header metadata, protected targets.h
 *   geometry key, and GEMC target-variation label normally used together for that target.
 *
 * Workflow:
 *   RunConfig reads `rgm-target` -> findRgmTarget() performs an exact catalog lookup -> fields whose
 *   individual setting is `auto` inherit catalog defaults -> explicit geometry, A/Z, or GEMC-variation
 *   overrides remain independent -> TargetGeometry validates/samples the selected external geometry.
 *
 * Scope:
 *   This catalog describes target identity and defaults; it does not implement vertex shapes, modify
 *   protected targets.h, load a GCARD, or make A/Z determine geometry implicitly.
 */

#pragma once
#include <string>
#include <vector>

namespace samples {

// Public RG-M target catalog --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public RG-M target catalog */

// RgmTarget object ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RG-M target record */
/**
 * @struct RgmTarget
 * @brief Material and assembly metadata for one supported RG-M target.
 *
 * Purpose:
 *   Keep the usual identity, nuclear metadata, vertex-geometry key, and detector-variation label in one
 *   readable passive record so automatic configuration remains explicit and reviewable.
 *
 * Creation and ownership:
 *   Records are aggregate-initialized once inside rgmTargets() and owned by its function-local static
 *   vector for process lifetime. Callers receive const references and cannot mutate catalog contents.
 *
 * Invariants and consumers:
 *   identifier is unique and used for exact lookup/help. A and Z satisfy the nuclear header contract.
 *   geometry is a key understood by the protected external targets.h adapter and is validated separately.
 *   gemc_variation is provenance/naming metadata for the matching detector target variation; this record
 *   does not open detector resources or require these four fields to remain coupled after CLI overrides.
 */
struct RgmTarget {
    std::string identifier;  ///< Stable, case-sensitive `--rgm-target` value shown in help and profiles.

    std::string description;  ///< Human-readable material/assembly description for maintainers.

    int A;  ///< Default LUND nuclear mass number; dimensionless and independent of vertex geometry.

    int Z;  ///< Default LUND nuclear charge number; dimensionless and independently overridable.

    std::string geometry;  ///< Default external targets.h vertex-distribution key.

    std::string gemc_variation;  ///< Default GEMC target-variation provenance/output-name token.
};
#pragma endregion

// Catalog access --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Catalog access */
/**
 * @brief Return the complete immutable RG-M target catalog in its maintained display order.
 *
 * @return Const reference to process-lifetime storage. The reference and its records remain valid until
 *         process termination and must not be cast to mutable access.
 *
 * @note Construction is lazy and thread-safe under the C++ function-local static initialization rules.
 */
const std::vector<RgmTarget>& rgmTargets();

/**
 * @brief Find one target record by exact user-facing identifier.
 *
 * @param identifier Case-sensitive identifier such as `Ar40`, `Ca48`, or `C12-four-foil`.
 *
 * @return Const reference to the matching process-lifetime catalog record.
 *
 * @throws std::runtime_error If no exact match exists; the diagnostic includes rgmTargetNames().
 */
const RgmTarget& findRgmTarget(const std::string& identifier);

/**
 * @brief Format supported identifiers for CLI help and lookup-error diagnostics.
 *
 * @return Newly owned comma-and-space-separated identifier list in rgmTargets() order.
 *
 * @note The returned text is presentation only and does not perform selection or validation.
 */
std::string rgmTargetNames();
#pragma endregion

#pragma endregion

}  // namespace samples
