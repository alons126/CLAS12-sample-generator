//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RgmTarget.h
 * @brief Lists the supported RG-M targets and their default settings.
 *
 * Purpose:
 *   Store the usual A, Z, targets.h geometry name, and GEMC variation for each RG-M target name.
 *
 * Workflow:
 *   RunConfig reads `rgm-target` -> findRgmTarget() finds the matching record -> the record supplies
 *   default geometry, A, Z, and GEMC variation -> explicit user overrides are applied afterward.
 *
 * Scope:
 *   This file stores names and defaults only. It does not define vertex shapes, change targets.h, load
 *   a GCARD, or choose geometry from A and Z.
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
 * @brief Default settings for one supported RG-M target.
 *
 * Purpose:
 *   Keep the target name, A, Z, vertex geometry, and GEMC variation together in one simple record.
 *
 * Creation and ownership:
 *   rgmTargets() creates the records once and stores them until the program ends. Callers receive
 *   read-only references and cannot change the catalog.
 *
 * Invariants and consumers:
 *   identifier is unique and comparisons are case-sensitive. A and Z are LUND header defaults. geometry
 *   is checked separately by TargetGeometry. gemc_variation is used in output names and the manifest.
 *   Users may override these defaults separately.
 */
struct RgmTarget {
    std::string identifier;  ///< Case-sensitive `--rgm-target` name shown in help and profiles.

    std::string description;  ///< Plain description of the target material and assembly.

    int A;  ///< Default target mass number written to the LUND header.

    int Z;  ///< Default target charge number written to the LUND header.

    std::string geometry;  ///< Default targets.h name used to sample vertices.

    std::string gemc_variation;  ///< Default GEMC target variation stored in names and the manifest.
};
#pragma endregion

// Catalog access --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Catalog access */
/**
 * @brief Return all supported RG-M targets in display order.
 *
 * @return Read-only reference to records that remain valid until the program ends.
 *
 * @note C++ creates the catalog on its first use and makes that initialization thread-safe.
 */
const std::vector<RgmTarget>& rgmTargets();

/**
 * @brief Find one target by its exact user-facing name.
 *
 * @param identifier Case-sensitive identifier such as `Ar40`, `Ca48`, or `C12-four-foil`.
 *
 * @return Read-only reference to the matching catalog record.
 *
 * @throws std::runtime_error If no exact match exists. The message lists the supported names.
 */
const RgmTarget& findRgmTarget(const std::string& identifier);

/**
 * @brief Join the supported target names for help and error messages.
 *
 * @return Comma-separated target names in rgmTargets() order.
 *
 * @note This function only builds text. It does not select or check a target.
 */
std::string rgmTargetNames();
#pragma endregion

#pragma endregion

}  // namespace samples
