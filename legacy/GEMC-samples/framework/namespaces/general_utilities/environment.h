//
// Created by Alon Sportes on 26/02/2026.
//

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <iostream>
#include <string_view>

/**
 * @namespace environment
 * @brief A namespace for defining color codes for terminal output, including error, completion, system, warning, and informational messages.
 * @details This namespace contains string variables that represent ANSI escape codes for different colors. These color codes can be used to format terminal output in a visually distinct
 * way, making it easier to identify different types of messages (e.g., errors in red, completions in green, warnings in magenta). The RESET_COLOR variable is used to reset the terminal
 * color back to the default after printing a colored message. By using these color codes consistently throughout the codebase, it can enhance readability and make it easier for developers
 * to quickly identify important information in the terminal output.
 * @note When using these color codes in terminal output, it is important to ensure that the terminal supports ANSI escape codes and that the output is not being redirected to a file or
 * other destination that may not interpret the escape codes correctly. Additionally, it is good practice to reset the color after printing a colored message to avoid unintended formatting
 * of subsequent output.
 */
namespace environment {
std::string ERROR_COLOR = "\033[31m";       // Red
std::string COMPLETION_COLOR = "\033[32m";  // Green
std::string SYSTEM_COLOR = "\033[33m";      // Yellow
std::string INFO_COLOR = "\033[35m";        // Magenta
std::string WARNING_COLOR = "\033[36m";     // Cyan
std::string RESET_COLOR = "\033[0m";        // Reset color
};  // namespace environment

#endif  // ENVIRONMENT_H
