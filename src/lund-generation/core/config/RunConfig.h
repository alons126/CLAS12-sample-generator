//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.h
 * @brief Public interface for reading and checking run settings.
 *
 * Purpose:
 *   Combine defaults, an optional profile, and command-line values into one checked configuration.
 *
 * Workflow:
 *   Call parse() once -> generators read strings or converted numbers -> the writer records every final
 *   value in the manifest.
 */

#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// RunConfig object ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig object */
/**
 * @class RunConfig
 * @brief Checked settings shared by the generator, converter, and writer.
 *
 * Purpose:
 *   Store every final key/value setting in one object. Generators can read numbers through checked
 *   conversion functions, and the manifest can record the same final text values.
 *
 * Role:
 *   RunConfig sits between the command-line programs and the LUND workflows. It defines accepted names,
 *   which source wins when values are repeated, how `auto` is resolved, which values are valid, and how
 *   final paths are named. Other components read this object instead of parsing settings again.
 *
 * Non-responsibilities:
 *   This object does not create events, read GST trees, sample random numbers, write LUND files, replace
 *   output directories, draw monitoring plots, or submit jobs. Those steps start only after parse()
 *   succeeds.
 *
 * Creation and workflow:
 *   1. parse() adds shared and source-specific defaults.
 *   2. It reads one optional profile and then applies command-line values.
 *   3. It replaces `auto` with the correct target, channel, beam, name, and path values.
 *   4. validate() rejects invalid combinations before output can be replaced.
 *   5. Generators read individual values, and the writer saves values() in the manifest.
 *
 * Units and representation:
 *   Values stay as strings so the manifest can record their exact final spelling. Momentum uses GeV/c,
 *   beam energy uses GeV, angles use degrees, vertices use cm, and counts, seeds, A, and Z are unsigned
 *   integers.
 *
 * Ownership and lifetime:
 *   Each RunConfig owns its map and can be copied or moved. Public functions do not change it after
 *   construction. A reference returned by values() is valid only while that RunConfig still exists.
 *
 * Invariants:
 *   A value returned by parse() contains every required key, no unknown keys, no unresolved automatic
 *   values, and an absolute output path. A default-constructed RunConfig is empty and must not be passed
 *   to a generator.
 */
class RunConfig {
   public:
    /**
     * @brief Build one complete and checked run configuration.
     *
     * @param argc Number of argv entries, including the executable name.
     * @param argv Process arguments read during this call. The object does not store argv.
     * @param uniform Select uniform-generation keys and defaults when true, or physical-conversion
     *                keys and defaults when false.
     *
     * @return Configuration ready for the generator, converter, and writer. All `auto` values are
     *         resolved and local paths are normalized.
     *
     * @throws std::exception For malformed, repeated, or unknown options; unreadable profiles; invalid
     *         values; unsupported modes; or path errors.
     */
    static RunConfig parse(int argc, char** argv, bool uniform);

    /**
     * @brief Return one final setting as text.
     * @param key Known source-specific or shared configuration key.
     * @return A copy of the stored value.
     * @throws std::out_of_range If the key is absent.
     */
    std::string get(const std::string& key) const;

    /**
     * @brief Read one complete setting as a finite double.
     * @param key Key whose documented unit remains the unit of the returned value.
     * @return The finite numeric value.
     * @throws std::exception If the key is absent or its complete value is not a finite number.
     */
    double number(const std::string& key) const;

    /**
     * @brief Read a digits-only setting as an unsigned 64-bit integer.
     * @param key Total count, per-file count, seed, or nuclear-metadata key to read.
     * @return The parsed unsigned value.
     * @throws std::exception If the key is absent, malformed, negative, or out of range.
     */
    std::uint64_t integer(const std::string& key) const;

    /**
     * @brief Return all final settings for the manifest.
     * @return Read-only reference owned by this RunConfig. Do not keep it after the object is destroyed.
     */
    const std::map<std::string, std::string>& values() const { return values_; }

    /**
     * @brief Check shared settings and the selected source mode before output creation.
     * @param uniform Check uniform rules when true or physical-conversion rules when false. This must
     *                match the mode used by parse().
     * @throws std::exception If a required value, range, relationship, target, or mode is invalid.
     */
    void validate(bool uniform) const;

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /**
     * @brief All final key/value settings owned by this object.
     *
     * parse() fixes the allowed keys for the selected source. Values stay as text for conversion and
     * manifest output. Other code receives copies or a read-only reference.
     */
    std::map<std::string, std::string> values_;
};
#pragma endregion

/**
 * @brief Build help text for one LUND application without running it.
 * @param uniform Select uniform-generator help when true or physical-converter help when false.
 * @return Shared options, source-specific options, units, and supported RG-M target names.
 */
std::string help(bool uniform);

/**
 * @brief Encode text as one complete JSON string.
 * @param text Unescaped source text.
 * @return Escaped text including surrounding double quotes.
 */
std::string jsonString(const std::string& text);
#pragma endregion

}  // namespace samples
