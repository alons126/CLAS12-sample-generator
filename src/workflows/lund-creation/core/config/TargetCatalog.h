//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file TargetCatalog.h
 * @brief Declares target metadata and beam-dependent GEMC target-variation selection.
 *
 * Purpose:
 *   Keep the user-facing target identity separate from its detector assembly. A target supplies default
 *   LUND A/Z metadata, while the target plus beam energy selects the usual GEMC variation and vertex
 *   geometry. An explicit GEMC variation can replace that automatic choice for exceptional runs.
 *
 * Workflow:
 *   RunConfig reads `target` and beam energy -> findTarget() checks the material ->
 *   resolveTargetVariation() chooses or validates the GEMC variation -> the resolved record supplies the
 *   targets.h geometry -> optional A/Z overrides are applied afterward.
 *
 * Scope:
 *   This interface selects metadata and geometry names. It does not sample vertices, load a GCARD, or
 *   infer target identity from A and Z.
 */

#pragma once
#include <string>
#include <vector>

namespace samples {

// Public target catalog -------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public target catalog */

// Target metadata -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Target metadata */
/**
 * @struct Target
 * @brief Nuclear metadata for one user-facing target identity.
 *
 * The identifier is accepted by `--target`. A and Z are the default LUND header values and may be
 * overridden independently for a controlled study.
 */
struct Target {
    std::string identifier;   ///< Case-sensitive material name such as `C12` or `Ar40`.
    std::string description;  ///< Plain description shown to developers.
    int A;                    ///< Default target mass number written to the LUND header.
    int Z;                    ///< Default target charge number written to the LUND header.
};
#pragma endregion

// Resolved target variation ---------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Resolved target variation */
/**
 * @struct TargetVariation
 * @brief One supported GEMC target variation and its vertex geometry.
 *
 * The variation identifier is recorded in the manifest and handed to simulation submission. Geometry is
 * the matching key in the protected targets.h implementation. target identifies the compatible material.
 */
struct TargetVariation {
    std::string identifier;  ///< GEMC target-variation name.
    std::string target;      ///< Compatible user-facing target identity.
    std::string geometry;    ///< targets.h key used to sample event vertices.
};
#pragma endregion

// Catalog access --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Catalog access */
/**
 * @brief Return all supported target identities in display order.
 * @return Read-only reference to records that remain valid until the program ends.
 */
const std::vector<Target>& targets();

/**
 * @brief Find one target by its exact user-facing identity.
 * @param identifier Case-sensitive identifier such as `H1`, `C12`, or `Ar40`.
 * @return Read-only reference to the matching catalog record.
 * @throws std::runtime_error If no exact match exists.
 */
const Target& findTarget(const std::string& identifier);

/**
 * @brief Resolve the detector variation and matching vertex geometry.
 *
 * @param target Checked target record.
 * @param beam_energy Beam energy in GeV.
 * @param override_name `auto` for the standard target/beam mapping, or an explicit compatible GEMC
 *                      variation such as `rgm_fall2021_C_S`.
 * @return Read-only reference to the selected variation record.
 * @throws std::runtime_error If no automatic mapping exists, or the explicit variation is unknown or
 *                            incompatible with the selected target.
 */
const TargetVariation& resolveTargetVariation(const Target& target, double beam_energy, const std::string& override_name);

/**
 * @brief Join the supported target identities for help and error messages.
 * @return Comma-separated target identities in catalog order.
 */
std::string targetNames();
#pragma endregion

#pragma endregion

}  // namespace samples
