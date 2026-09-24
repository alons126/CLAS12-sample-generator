//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file environment.h
 * @brief C++ access to the launcher-owned terminal-presentation palette.
 *
 * Purpose:
 *   Give generators, converters, writers, and application entry points semantic color names while
 *   keeping the actual palette centralized in the sourced set_colors.csh launcher helper.
 *
 * Workflow:
 *   set_colors.csh exports literal `\033` `*_COLOR` values -> the launcher starts a C++ application ->
 *   this header reads and decodes those inherited values once -> output code uses the semantic names.
 *
 * Failure behavior:
 *   A missing `*_COLOR` variable resolves to an empty string, leaving output uncolored when an executable
 *   is invoked outside the maintained launcher. No independent fallback palette can drift from the
 *   shell-owned definitions.
 */

#pragma once

#include <cstdlib>
#include <string>

// Terminal presentation constants ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Terminal presentation constants */

/**
 * @namespace environment
 * @brief Semantic colors inherited from the launcher environment by maintained C++ output.
 *
 * Lifetime and ownership:
 *   Every inline const string owns its decoded value for program lifetime. The process environment is
 *   read once during initialization; later environment changes do not alter an application's palette.
 *
 * Invariants:
 *   Names map directly to the `*_COLOR` contract exported by set_colors.csh. RESET_COLOR must follow
 *   colored content so later terminal output is unaffected.
 *
 * Assumptions:
 *   The shell palette stores escape bytes as a literal `\033` prefix so tcsh can export it reliably.
 *   Already-decoded environment content is preserved, and redirected output retains any control bytes.
 */
namespace environment {

// Environment decoding --------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Environment decoding */
/**
 * @brief Read and decode one color exported by set_colors.csh.
 *
 * @param variable `*_COLOR` environment-variable name owned by the shell palette.
 * @return Owned terminal sequence, or an empty string when the variable is unavailable.
 *
 * @note Every literal `\033` token is replaced with one escape byte, matching the Python launcher's
 *       interpretation of the same inherited values. Other content is retained verbatim.
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

#pragma region /* Semantic palette */
inline const std::string ERROR_COLOR = inheritedColor("ERROR_COLOR");            ///< Error/failure text.
inline const std::string COMPLETION_COLOR = inheritedColor("COMPLETION_COLOR");  ///< Successful-completion text.
inline const std::string SYSTEM_COLOR = inheritedColor("SYSTEM_COLOR");          ///< Workflow/system text.
inline const std::string INFO_COLOR = inheritedColor("INFO_COLOR");              ///< Informational text.
inline const std::string WARNING_COLOR = inheritedColor("WARNING_COLOR");        ///< Warning/replacement text.
inline const std::string RESET_COLOR = inheritedColor("RESET_COLOR");            ///< Restore terminal styling.
#pragma endregion

}  // namespace environment

#pragma endregion
