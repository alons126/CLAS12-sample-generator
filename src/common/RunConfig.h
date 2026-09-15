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

// Strict key/value configuration: file values, then command-line overrides.
// RunConfig object ------------------------------------------------
#pragma region /* RunConfig object */
/**
 * @class RunConfig
 * @brief Validated settings shared by the generator, converter and writer.
 *
 * Usage order:
 *   1. parse merges defaults, one optional file and CLI overrides.
 *   2. Automatic sampling settings are resolved and validate checks constraints.
 *   3. get/number/integer supply settings; values supplies manifest provenance.
 */
class RunConfig {
   public:
    /** @brief Merge and validate options; genie selects the conversion option set. */
    static RunConfig parse(int argc, char** argv, bool genie);
    /** @brief Return a known setting as text; a missing key throws. */
    std::string get(const std::string& key) const;
    /** @brief Convert a setting to a finite double, rejecting trailing text. */
    double number(const std::string& key) const;
    /** @brief Convert decimal digits to an unsigned integer with overflow checks. */
    std::uint64_t integer(const std::string& key) const;
    /** @brief Expose resolved settings by const reference for provenance serialization. */
    const std::map<std::string, std::string>& values() const { return values_; }
    /** @brief Check shared and workflow-specific constraints; throw before output creation. */
    void validate(bool genie) const;

    // Owned state --------------------------------------------------------------
   private:
    // parse owns and resolves this map; downstream readers receive values or a const view.
    std::map<std::string, std::string> values_;
};
#pragma endregion

/** @brief Return the selected application usage text without executing a run. */
std::string help(bool genie);

/** @brief Escape text and enclose it in quotes for JSON serialization. */
std::string jsonString(const std::string& text);
#pragma endregion
}  // namespace samples
