//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.h
 * @brief Reads run settings, checks them, and makes their final values available to the LUND workflow.
 *
 * Purpose:
 *   A setting may come from a built-in default, a configuration file, or the command line. RunConfig
 *   combines those sources into one result. For example, `--events 1000` on the command line replaces an
 *   `events` value in the configuration file, which would otherwise replace the built-in default.
 *
 * Execution flow:
 *   Call createFromCommandLine() once before creating output. It reads every setting, fills values marked
 *   `auto`, checks the result, and prepares absolute paths. Event-producing code then reads the settings,
 *   and the writer records the same final values in the run manifest.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

namespace samples {

// Run configuration interface -------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Run configuration interface */

// LUND source -----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LUND source */
/**
 * @enum LundSource
 * @brief Identifies which LUND-creation path owns a set of settings.
 *
 * createFromCommandLine(), validateForSource(), and buildHelpText() use this value to choose the accepted
 * settings and rules. A named value makes each call state its purpose instead of using an unclear true or
 * false argument.
 */
enum class LundSource {
    Uniform,  ///< Settings for the uniform LUND creator.
    Physical  ///< Settings for the physical LUND converter.
};
#pragma endregion

// RunConfig class -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig class */
/**
 * @class RunConfig
 * @brief Stores the final checked settings for one LUND-creation run.
 *
 * Purpose:
 *   Keep every setting in one object so all workflow parts use the same values. Settings remain stored as
 *   text for the manifest. getDouble() and getNonnegativeInteger() check and convert text when code needs a
 *   numeric value.
 *
 * Use:
 *   Use createFromCommandLine() to create the object. It applies command-line values over
 *   configuration-file values and configuration-file values over defaults. It then replaces `auto`,
 *   checks every setting, and builds the final paths. Other code only reads the completed object.
 *
 * Scope:
 *   RunConfig only prepares settings. It does not create events, choose random values, write output, make
 *   plots, or submit simulation jobs.
 *
 * Execution flow:
 *   Start with built-in defaults. Read one optional configuration file. Apply command-line values last.
 *   Calculate automatic values, check the complete result, and make it available to the workflow.
 *
 * Stored values:
 *   Values stay as strings so the manifest records their exact final spelling. Beam energy is measured in GeV,
 *   momentum in GeV/c, angles in degrees, and target positions in centimeters. Counts, random seeds, A, and Z
 *   are whole numbers that cannot be negative.
 *
 * Ownership and lifetime:
 *   Each RunConfig owns its settings and may be copied or moved. Its public functions do not change those
 *   settings after createFromCommandLine() returns. The map returned by getAllSettings() belongs to the
 *   RunConfig and must not be used after that object is destroyed.
 *
 * Rules:
 *   A RunConfig returned by createFromCommandLine() contains every required setting, contains no unknown
 *   settings or remaining `auto` values, and has an absolute output path. Event-producing code must use
 *   this completed object rather than an empty default-constructed one.
 */
class RunConfig {
   public:
    /**
     * @brief Read all setting sources and return one complete, checked configuration.
     * @param argc Number of argv entries, including the executable name.
     * @param argv Command-line arguments read during this call. The returned object does not store argv.
     * @param source LundSource::Uniform for the uniform LUND creator or LundSource::Physical for the
     *               physical LUND converter.
     * @return Configuration ready for event creation and LUND writing. Every `auto` value has been
     *         replaced, and local paths have been made absolute and cleaned of `.` and `..` parts.
     * @throws std::exception If an option is malformed, repeated, or unknown; the configuration file
     *                        cannot be read; a value or mode is invalid; or a path cannot be prepared.
     */
    static RunConfig createFromCommandLine(int argc, char** argv, LundSource source);

    /**
     * @brief Get one final setting without converting its text.
     * @param key Exact setting name.
     * @return A copy of the setting's stored text.
     * @throws std::out_of_range If the configuration does not contain this name.
     */
    std::string getText(const std::string& key) const;

    /**
     * @brief Convert one setting to a number that may contain a decimal point.
     * @param key Exact setting name. The result keeps the unit documented for that setting.
     * @return The converted value.
     * @throws std::exception If the setting is missing, contains extra text, or is infinite or not a
     *                        number.
     */
    double getDouble(const std::string& key) const;

    /**
     * @brief Convert one setting made only of digits to a nonnegative whole number.
     * @param key Exact setting name, normally a count, seed, A, or Z.
     * @return The converted whole number.
     * @throws std::exception If the setting is missing, empty, contains anything except digits, or is too
     *                        large for a 64-bit unsigned integer.
     */
    std::uint64_t getNonnegativeInteger(const std::string& key) const;

    /**
     * @brief Get every final setting so the writer can record the run manifest.
     * @return The complete key/value map. The caller may read it but not change it. The map remains valid
     *         only while this RunConfig exists.
     */
    const std::map<std::string, std::string>& getAllSettings() const { return values_; }

    /**
     * @brief Check every setting needed by the selected kind of event source.
     * @param source LundSource::Uniform to check the uniform LUND creator's rules or LundSource::Physical
     *               to check the physical LUND converter's rules. Use the same value passed to
     *               createFromCommandLine().
     * @throws std::exception If a required setting is missing or any value, range, target, or combination
     *                        of settings is invalid.
     */
    void validateForSource(LundSource source) const;

    // Stored settings ---------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /**
     * @brief Complete setting names and their final text values.
     *
     * createFromCommandLine() decides which names are allowed for the selected LUND source. Text is kept
     * unchanged after automatic values and paths are resolved so the same values can be converted for code
     * and written to the manifest.
     */
    std::map<std::string, std::string> values_;
};
#pragma endregion

/**
 * @brief Build the command-line help text for one kind of LUND creation.
 * @param source LundSource::Uniform for uniform LUND creator help or LundSource::Physical for physical
 *               LUND converter help.
 * @return Text listing the command, shared settings, source-specific settings, units, and supported
 *         target names.
 */
std::string buildHelpText(LundSource source);

/**
 * @brief Prepare text for safe use as one quoted JSON value.
 * @param text Original text.
 * @return A JSON string including its surrounding double quotes. Quotes, backslashes, and invisible
 *         control characters inside the text are written in JSON's escaped form.
 */
std::string quoteAsJsonString(const std::string& text);
#pragma endregion

}  // namespace samples
