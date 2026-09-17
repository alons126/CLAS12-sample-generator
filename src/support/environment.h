//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file environment.h
 * @brief Central terminal-presentation constants for maintained C++ applications.
 *
 * Purpose:
 *   Keep ANSI control sequences out of generators, converters, writers, and application entry points.
 *   Consumers select a semantic color name and return to RESET_COLOR after the emphasized text.
 *
 * Workflow:
 *   Include this header -> stream one semantic color before a message -> stream RESET_COLOR before
 *   ordinary output resumes. The constants own no state and perform no terminal detection or I/O.
 *
 * Scope:
 *   This is the only maintained C++ source of terminal color definitions. Shell launchers have their
 *   own environment-variable palette because they cannot include a C++ header.
 */

#pragma once

#include <string_view>

// Terminal presentation constants ------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Terminal presentation constants */

/**
 * @namespace environment
 * @brief Semantic ANSI foreground colors shared by maintained C++ terminal output.
 *
 * Lifetime and ownership:
 *   Every value is an inline constexpr string view over static program storage. Consumers borrow the
 *   view without allocation, mutation, initialization order, or lifetime concerns.
 *
 * Invariants:
 *   Each non-reset value begins one ANSI SGR foreground style. RESET_COLOR restores default terminal
 *   presentation and must follow colored content so later output is unaffected.
 *
 * Assumptions:
 *   Output is intended for ANSI-capable terminals and batch logs. Redirected output retains control
 *   bytes; this header deliberately provides presentation constants rather than terminal detection.
 */
namespace environment {
inline constexpr std::string_view ERROR_COLOR = "\033[31m";       ///< Red error/failure text.
inline constexpr std::string_view COMPLETION_COLOR = "\033[32m";  ///< Green successful-completion text.
inline constexpr std::string_view SYSTEM_COLOR = "\033[33m";      ///< Yellow workflow/system text.
inline constexpr std::string_view INFO_COLOR = "\033[35m";        ///< Magenta informational text.
inline constexpr std::string_view WARNING_COLOR = "\033[36m";     ///< Cyan warning/replacement text.
inline constexpr std::string_view RESET_COLOR = "\033[0m";        ///< Restore default terminal styling.
}  // namespace environment

#pragma endregion
