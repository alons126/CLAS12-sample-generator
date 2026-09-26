//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file environment.h
 * @brief Reads the project-wide terminal colors set by the shell support layer.
 *
 * Purpose:
 *   Give C++ output clear color names while keeping the actual color values in set_colors.csh.
 *
 * Workflow:
 *   set_colors.csh exports `*_COLOR` variables -> this header reads them when the program starts ->
 *   inheritedColor() changes each written `\033` marker into an escape byte -> output code uses the
 *   named color strings below.
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
 * @brief Color strings used by maintained C++ terminal output.
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
 *   The shell writes the escape code as the four characters `\033`. Text that already contains a real
 *   escape byte is left unchanged.
 */
namespace environment {

// Environment decoding --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Environment decoding */
/**
 * @brief Read one project color and make it ready for terminal output.
 * @param variable Name of the `*_COLOR` environment variable to read.
 * @return The decoded color string, or an empty string when the variable is missing.
 * @note Replaces every written `\033` marker with one escape byte. All other characters stay unchanged.
 */
inline std::string inheritedColor(const char* variable) {
    const char* inherited = std::getenv(variable);

    if (inherited == nullptr) { return {}; }

    std::string color(inherited);
    constexpr const char* encoded_escape = "\\033";
    std::string::size_type position = 0;

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
