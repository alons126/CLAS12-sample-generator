//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.h
 * @brief Public sample-configuration interface.
 *
 * Purpose:
 *   Merge defaults, a sample profile and CLI overrides into validated settings.
 *
 * Workflow:
 *   Call parse once; generators read typed values; the writer records values in the manifest.
 */

#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// RunConfig object ------------------------------------------------
#pragma region /* RunConfig object */
/**
 * @class RunConfig
 * @brief Validated settings shared by the generator, converter and writer.
 *
 * Purpose:
 *   Provide one typed access boundary over the strict key/value configuration used by uniform LUND
 *   generation and physical event conversion. Keeping the resolved strings together ensures physics
 *   code and manifest provenance observe the same final settings.
 *
 * Architectural role:
 *   RunConfig is the handoff between the command-line applications and the LUND workflows. It owns
 *   policy for accepted setting names, precedence, automatic-value resolution, validation, and final
 *   path/name construction. UniformGenerator, physical adapters, and LundWriter consume the resulting
 *   read-only value object instead of reparsing arguments or independently deriving configuration.
 *
 * Non-responsibilities:
 *   This object does not sample or convert events, own random-number generators, inspect GST trees,
 *   serialize LUND records, create or replace output directories, render monitoring, or submit detector
 *   jobs. Those effects begin only after parse() has returned successfully and belong to the selected
 *   generator/converter and writer.
 *
 * Creation and workflow:
 *   1. parse() installs shared and source-specific defaults.
 *   2. It applies one optional profile, followed by command-line overrides.
 *   3. It resolves automatic target, channel, beam, naming, and path values.
 *   4. validate() rejects inconsistent settings before an output directory can be replaced.
 *   5. Generators read individual typed values; the writer serializes values() as provenance.
 *
 * Units and representation:
 *   Values remain strings so their resolved spelling can be recorded exactly. Individual key
 *   contracts define units: momenta and beam energy are GeV, angles are degrees, vertices are cm,
 *   counts/seeds/A/Z are unsigned integers, and paths are normalized strings.
 *
 * Ownership and lifetime:
 *   Each RunConfig owns its map by value and may be copied or moved normally. Public access is
 *   read-only after construction. References returned by values() remain valid while that object is
 *   alive and unchanged; consumers must not retain them beyond the configuration lifetime.
 *
 * Invariants:
 *   An instance returned by parse() contains every key required by its selected source mode, contains
 *   no unknown keys, has resolved automatic values, satisfies validate(), and has an absolute output
 *   path. Consumers must obtain operational configurations from parse(); a default-constructed object
 *   has an empty map and is not valid input to a generator.
 */
class RunConfig {
   public:
    /**
     * @brief Construct one complete, validated run configuration.
     *
     * @param argc Number of argv entries, including the executable name.
     * @param argv Borrowed process arguments read during this call; storage is not retained.
     * @param uniform Select uniform-generation keys and defaults when true, or physical-conversion
     *                keys and defaults when false.
     *
     * @return An owning, read-only configuration ready for generator, adapter, and writer consumption;
     *         all stored `auto` values have been resolved and local operational paths normalized.
     *
     * @throws std::exception For malformed/repeated/unknown options, unreadable profiles, invalid
     *         values, unsupported source settings, path failures, or unsafe/incomplete run metadata.
     */
    static RunConfig parse(int argc, char** argv, bool uniform);

    /**
     * @brief Return one resolved setting in its provenance-preserving text form.
     * @param key Known source-specific or shared configuration key.
     * @return A copy of the stored value.
     * @throws std::out_of_range If the key is absent.
     */
    std::string get(const std::string& key) const;

    /**
     * @brief Convert a resolved setting to a finite double without accepting trailing text.
     * @param key Key whose documented unit remains the unit of the returned value.
     * @return The finite numeric value.
     * @throws std::exception If the key is absent or its complete value is not a finite number.
     */
    double number(const std::string& key) const;

    /**
     * @brief Convert a decimal-digits-only setting to an unsigned 64-bit integer.
     * @param key Count, seed, or nuclear-metadata key to read.
     * @return The parsed unsigned value.
     * @throws std::exception If the key is absent, malformed, negative, or out of range.
     */
    std::uint64_t integer(const std::string& key) const;

    /**
     * @brief Expose all resolved settings for deterministic provenance serialization.
     * @return Const view owned by this RunConfig; callers must not outlive this object.
     */
    const std::map<std::string, std::string>& values() const { return values_; }

    /**
     * @brief Recheck shared and selected-source invariants before output creation.
     * @param uniform Apply uniform constraints when true or physical-conversion constraints when false;
     *                this must match the mode used to create the object.
     * @throws std::exception If a required value, range, relationship, target, or mode is invalid.
     */
    void validate(bool uniform) const;

    // Owned state --------------------------------------------------------------
   private:
    /**
     * @brief Complete resolved key/value state owned by this object.
     *
     * Keys are fixed by parse() for the selected source. Values retain their final text form for both
     * typed access and manifest output; downstream code receives copies or a const view only.
     */
    std::map<std::string, std::string> values_;
};
#pragma endregion

/**
 * @brief Build usage text for one LUND source application without executing it.
 * @param uniform Select uniform-generator help when true or physical-converter help when false.
 * @return Shared options, source-specific options, units, and supported RG-M target names.
 */
std::string help(bool uniform);

/**
 * @brief Encode arbitrary provenance text as one complete JSON string literal.
 * @param text Unescaped source text.
 * @return Escaped text including surrounding double quotes.
 */
std::string jsonString(const std::string& text);
#pragma endregion
}  // namespace samples
