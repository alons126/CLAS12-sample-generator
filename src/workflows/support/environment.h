//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file environment.h
 * @brief Provides the project-wide terminal colors for C++ output.
 *
 * Purpose:
 *   Give C++ output clear color names while keeping the actual color values in set_colors.csh.
 *
 * Execution flow:
 *   set_colors.csh stores colors as text. For example, `\033[31m` means "make the following text red."
 *   C++ reads that environment value as the ordinary characters `\`, `0`, `3`, `3`, `[`, `3`, `1`, and
 *   `m`. A terminal does not recognize those ordinary characters as a color instruction. It expects the
 *   instruction to begin with one special, invisible character called the escape character.
 *   inheritedColor() replaces each `\033` marker with that one character. The terminal
 *   can then recognize the instruction and color the text printed after it.
 *
 * Failure behavior:
 *   If a color variable is missing, its value is an empty string and the output stays uncolored. This
 *   header does not define a second set of fallback colors.
 */

#pragma once

#include <cstdlib>
#include <string>

// Terminal presentation constants ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Terminal presentation constants */

/**
 * @namespace environment
 * @brief Color strings used by project C++ terminal output.
 *
 * Lifetime and ownership:
 *   Each inline string stores its value for the life of the program. The variables are read during
 *   program startup, so changing the environment later does not change these strings.
 *
 * Invariants:
 *   Each C++ name reads the matching `*_COLOR` variable from set_colors.csh. Output must use RESET_COLOR
 *   after colored text so the following terminal text returns to its normal style.
 *
 * Assumptions:
 *   The shell writes the escape character as the four ordinary characters `\033`. A value that already
 *   contains the special escape character is left unchanged.
 */
namespace environment {

// Environment decoding --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Environment decoding */
/**
 * @brief Read one project color and make it ready for terminal output.
 * @param variable Name of the `*_COLOR` environment variable to read.
 * @return The decoded color string, or an empty string when the variable is missing.
 * @note Replaces the four ordinary characters `\033` with the one invisible escape character that tells
 * the terminal, "the following characters are a formatting instruction." Without this replacement, a
 * value such as `\033[31m` would not make later text red. This replacement is needed here because
 * getenv() reads ordinary text. When `\033` appears directly in a C++ string literal, the compiler makes
 * this replacement automatically.
 */
inline std::string inheritedColor(const char* variable) {
    // Ask the operating system for the text stored under this environment-variable name. getenv()
    // returns nullptr when no value was exported, so an absent color becomes an empty string.
    const char* inherited = std::getenv(variable);
    if (inherited == nullptr) { return {}; }

    // Copy the received text into a string that this function can change. Search for the four ordinary
    // characters `\033`, starting at the beginning of the string.
    std::string color(inherited);
    constexpr const char* encoded_escape = "\\033";
    std::string::size_type position = 0;

    // Replace every `\033` marker with character number 27, the invisible escape character expected by
    // the terminal. Resume after the inserted character so the search can find any later markers.
    while ((position = color.find(encoded_escape, position)) != std::string::npos) {
        color.replace(position, 4, 1, static_cast<char>(27));
        ++position;
    }

    return color;
}
#pragma endregion

// Semantic palette ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region                                                                   /* Semantic palette */
inline const std::string ERROR_COLOR = inheritedColor("ERROR_COLOR");            ///< Error/failure text.
inline const std::string COMPLETION_COLOR = inheritedColor("COMPLETION_COLOR");  ///< Successful-completion text.
inline const std::string SYSTEM_COLOR = inheritedColor("SYSTEM_COLOR");          ///< Workflow/system text.
inline const std::string INFO_COLOR = inheritedColor("INFO_COLOR");              ///< Informational text.
inline const std::string WARNING_COLOR = inheritedColor("WARNING_COLOR");        ///< Warning/replacement text.
inline const std::string RESET_COLOR = inheritedColor("RESET_COLOR");            ///< Restore terminal styling.
#pragma endregion

}  // namespace environment

#pragma endregion
