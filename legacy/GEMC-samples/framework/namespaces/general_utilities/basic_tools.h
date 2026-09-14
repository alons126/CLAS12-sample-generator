//
// Created by Alon Sportes on 23/03/2025.
//

#ifndef BASIC_TOOLS_H
#define BASIC_TOOLS_H

#include <limits.h>  // for PATH_MAX
#include <netdb.h>   // for getaddrinfo/getnameinfo
#include <sys/socket.h>
#include <unistd.h>  // for getcwd

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>  // for getenv
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>  // for std::ostringstream
#include <string>
#include <utility>
#include <vector>

// include environment for colored output:
#include "TString.h"

// Include libraries:
#include "environment.h"

namespace env = environment;

/**
 * @namespace basic_tools
 *
 * Lowest-level utility namespace used across the analysis framework.
 *
 * Purpose:
 *   Collect tiny, broadly reusable helpers that are safe to include almost anywhere,
 *   including formatting/print helpers, simple string utilities, beam-energy parsing,
 *   filesystem checks, current-directory lookup, SSH-environment inspection, and
 *   skipped-HIPO-file logging.
 *
 * Design notes:
 *   - This namespace is intended to stay dependency-light.
 *   - It should not depend on higher-level analysis classes or namespaces.
 *   - Current non-standard-library dependencies are limited to `environment.h`
 *     for colored terminal output and `TString.h` for HIPO-file logging helpers.
 */
namespace basic_tools {

// Forward declaration for helpers used before their definitions in this header.
inline std::string TrimWhitespace(std::string value);

/**
 * @brief Normalize a source-file path for readable printouts.
 *
 * This performs lexical normalization only (for example, removing `.` and
 * collapsing `..` segments) and does not require the file to exist.
 */
inline std::string NormalizeFilePathForPrint(const std::string& fileName) {
    if (fileName.empty()) { return fileName; }
    return std::filesystem::path(fileName).lexically_normal().string();
}

// ======================================================================================================================================================================
// Printout functions
// ======================================================================================================================================================================

// PrintLine function ---------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a horizontal separator line.
 *
 * @param os Output stream to write to.
 * @param fill Character used to build the line.
 * @param width Number of repeated characters to print.
 *
 * This is the most basic line-print helper used by the higher-level banner and
 * section-header functions below.
 */
inline void PrintLine(std::ostream& os, const char fill = '-', const int width = 91) {
    os << std::string(width, fill) << "\n";  // Print a repeated fill character followed by a newline
}

// PrintLineColored function --------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a colored horizontal separator line.
 *
 * @param fill Character used to build the line.
 * @param width Number of repeated characters to print.
 * @param color ANSI-style color prefix taken from `environment`.
 * @param os Output stream to write to.
 *
 * The function sets the requested color, delegates the actual line creation to
 * `PrintLine`, and then restores the terminal color.
 */
inline void PrintLineColored(const char fill = '-', const int width = 91, const std::string& color = env::SYSTEM_COLOR, std::ostream& os = std::cout) {
    os << color;                 // Set terminal color
    PrintLine(os, fill, width);  // Delegate to PrintLine for the actual line
    os << env::RESET_COLOR;      // Restore terminal color
}

// PrintTitleBlock function ---------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a boxed title block made of two separator lines and a title line.
 *
 * @param title Title text to display.
 * @param fill Character used for the border lines.
 * @param width Total printed width.
 * @param color ANSI-style color prefix taken from `environment`.
 * @param os Output stream to write to.
 *
 * This helper is useful for clearly marking a major execution stage in terminal output.
 */
inline void PrintTitleBlock(const std::string& title, const char fill = '=', const int width = 91, const std::string& color = env::SYSTEM_COLOR, std::ostream& os = std::cout) {
    PrintLineColored(fill, width, color, os);                                           // Print top border
    os << color << std::left << std::setw(width) << title << env::RESET_COLOR << "\n";  // Print the title line
    PrintLineColored(fill, width, color, os);                                           // Print bottom border
    os << "\n";                                                                         // Add a blank line after the block
}

// PrintSectionHeader function ------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a section header surrounded by separator lines.
 *
 * @param section Section label to display.
 * @param width Total printed width.
 * @param color ANSI-style color prefix taken from `environment`.
 * @param os Output stream to write to.
 *
 * Compared with `PrintTitleBlock`, this is a slightly lighter-weight formatter for
 * ordinary sections inside larger terminal printouts.
 */
inline void PrintSectionHeader(const std::string& section, const int width = 91, const std::string& color = env::SYSTEM_COLOR, std::ostream& os = std::cout) {
    PrintLineColored('-', width, color, os);                                              // Top separator
    os << color << std::left << std::setw(width) << section << env::RESET_COLOR << "\n";  // Section label
    PrintLineColored('-', width, color, os);                                              // Bottom separator
}

// PrintBanner function -------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a centered banner with top and bottom borders.
 *
 * @param label Text to center inside the banner.
 * @param width Total banner width.
 * @param borderChar Character used for the border line.
 * @param borderColor ANSI-style color prefix for the border.
 * @param labelColor ANSI-style color prefix for the label text.
 * @param os Output stream to write to.
 *
 * The label is truncated if needed so the banner always fits inside the requested width.
 */
inline void PrintBanner(const std::string& label, const int width = 91, const char borderChar = '/', const std::string& borderColor = env::WARNING_COLOR,
                        const std::string& labelColor = env::RESET_COLOR, std::ostream& os = std::cout) {
    const int safeWidth = std::max(width, 4);  // Ensure minimum width for the banner
    const int innerWidth = safeWidth - 4;      // Reserve space for the // border

    std::string trimmedLabel = label;                                                                               // Copy and possibly truncate label
    if (static_cast<int>(trimmedLabel.size()) > innerWidth) { trimmedLabel = trimmedLabel.substr(0, innerWidth); }  // Truncate label if too long

    const int leftPad = (innerWidth - static_cast<int>(trimmedLabel.size())) / 2;  // Pad to center label
    const int rightPad = innerWidth - static_cast<int>(trimmedLabel.size()) - leftPad;
    const std::string borderLine(safeWidth, borderChar);  // Build the border line

    os << borderColor << borderLine << "\n";                                                                                                      // Print top border
    os << borderColor << "//" << std::string(leftPad, ' ') << labelColor << trimmedLabel << borderColor << std::string(rightPad, ' ') << "//\n";  // Print centered label
    os << borderColor << borderLine << "\n" << env::RESET_COLOR << std::flush;                                                                    // Print bottom border and flush output
}

// PrintAlignedValue function -------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a `label: value` pair with right-aligned value placement.
 *
 * @tparam T Printable value type.
 * @param os Output stream to write to.
 * @param label Label shown on the left.
 * @param value Value shown on the right.
 * @param lineWidth Target total line width.
 * @param minGap Minimum number of spaces kept between label and value.
 *
 * If the value is too long to keep the requested alignment, the function falls back
 * to a simpler single-space separation.
 */
template <typename T>
inline void PrintAlignedValue(std::ostream& os, const std::string& label, const T& value, const int lineWidth = 91, const int minGap = 1) {
    std::ostringstream valueStream;  // Stream for value serialization
    valueStream << value;
    const std::string valueStr = valueStream.str();  // Extract value as string
    const std::string prefix = label + ":";          // Build label prefix

    const int spaces = lineWidth - static_cast<int>(prefix.size()) - static_cast<int>(valueStr.size());  // Calculate available spacing
    if (spaces < minGap) {
        os << prefix << " " << valueStr << "\n";  // Fallback: single space if not enough room
        return;
    }

    os << prefix << std::string(spaces, ' ') << env::RESET_COLOR << valueStr << "\n";  // Print aligned value
}

// PrintAlignedValueColored function ------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Colored wrapper around `PrintAlignedValue`.
 *
 * @tparam T Printable value type.
 * @param label Label shown on the left.
 * @param value Value shown on the right.
 * @param lineWidth Target total line width.
 * @param minGap Minimum number of spaces kept between label and value.
 * @param color ANSI-style color prefix taken from `environment`.
 * @param os Output stream to write to.
 */
template <typename T>
inline void PrintAlignedValueColored(const std::string& label, const T& value, const int lineWidth = 91, const int minGap = 1, const std::string& color = env::SYSTEM_COLOR,
                                     std::ostream& os = std::cout) {
    os << color;                                             // Set color
    PrintAlignedValue(os, label, value, lineWidth, minGap);  // Delegate to PrintAlignedValue
    os << env::RESET_COLOR;                                  // Reset color
}

// PrintWrappedLabelValue function -------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a `label: value` pair while wrapping long values across multiple lines.
 *
 * @tparam T Printable value type.
 * @param os Output stream to write to.
 * @param label Label shown on the first line.
 * @param value Value to print.
 * @param lineWidth Target total line width.
 * @param spacing Number of spaces between `label:` and the first chunk of the value.
 *
 * Continuation lines are indented so the wrapped value starts underneath the first
 * value character rather than underneath the label.
 */
template <typename T>
inline void PrintWrappedLabelValue(std::ostream& os, const std::string& label, const T& value, const int lineWidth = 91, const int spacing = 1) {
    std::ostringstream valueStream;  // Serialize value to a stream
    valueStream << value;
    const std::string valueStr = valueStream.str();  // Extract value string
    const std::string prefix = label + ":";          // Build label prefix

    const int firstIndent = static_cast<int>(prefix.size()) + spacing;  // Indent for first line
    const int continueIndent = firstIndent;                             // Indent for wrapped lines
    const std::string continuationPrefix(continueIndent, ' ');          // Build indentation for wrapped lines
    const int firstAvail = std::max(1, lineWidth - firstIndent);        // Space available for first line value
    const int nextAvail = std::max(1, lineWidth - continueIndent);      // Space available for subsequent lines

    if (valueStr.empty()) {
        os << prefix << std::string(spacing, ' ') << "\n";  // Print label if value is empty
        return;
    }

    size_t pos = 0;  // Track position in value string
    bool firstLine = true;
    while (pos < valueStr.size()) {
        const int avail = firstLine ? firstAvail : nextAvail;                                 // Use correct width for first/next lines
        const size_t chunkLen = std::min(static_cast<size_t>(avail), valueStr.size() - pos);  // Compute chunk length
        const std::string chunk = valueStr.substr(pos, chunkLen);                             // Extract chunk to print

        if (firstLine) {
            os << prefix << std::string(spacing, ' ') << env::RESET_COLOR << chunk << "\n";  // Print first line
            firstLine = false;
        } else {
            os << continuationPrefix << env::RESET_COLOR << chunk << "\n";  // Print wrapped continuation line
        }
        pos += chunkLen;  // Advance position
    }
}

// PrintWrappedLabelValueColored function -------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Colored wrapper around `PrintWrappedLabelValue`.
 *
 * @tparam T Printable value type.
 * @param label Label shown on the first line.
 * @param value Value to print.
 * @param lineWidth Target total line width.
 * @param spacing Number of spaces between `label:` and the first chunk of the value.
 * @param color ANSI-style color prefix taken from `environment`.
 * @param os Output stream to write to.
 */
template <typename T>
inline void PrintWrappedLabelValueColored(const std::string& label, const T& value, const int lineWidth = 91, const int spacing = 1, const std::string& color = env::SYSTEM_COLOR,
                                          std::ostream& os = std::cout) {
    os << color;                                                   // Set color
    PrintWrappedLabelValue(os, label, value, lineWidth, spacing);  // Delegate to PrintWrappedLabelValue
    os << env::RESET_COLOR;                                        // Reset color
}

// PrintMessage function -----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a standalone message in the framework system color.
 *
 * @param Message Message text to print.
 *
 * A leading blank line is emitted before the message to make it stand out in logs.
 */
inline void PrintMessage(const std::string& Message) {
    std::cout << env::SYSTEM_COLOR << "\n" << Message << "\n" << env::RESET_COLOR << std::flush;  // Print colored message with leading blank line
}

// ExitWithError function -----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a formatted fatal error message and terminate the program.
 *
 * @param functionName Name of the function reporting the error.
 * @param fileName Source file where the error occurred.
 * @param lineNumber Source line where the error occurred.
 * @param errorMessage Human-readable error description.
 *
 * This is the namespace-level hard-stop helper used when execution should not continue.
 */
[[noreturn]] inline void ExitWithError(const std::string& functionName, const std::string& fileName, int lineNumber, const std::string& errorMessage) {
    const std::string normalizedFileName = NormalizeFilePathForPrint(fileName);
    std::cerr << "\n" << std::endl;                                                                                                         // Separate error visually with a blank line
    std::cerr << env::SYSTEM_COLOR << "[" << functionName << "]" << ": " << env::ERROR_COLOR << "Error!" << env::RESET_COLOR << std::endl;  // Header
    std::cerr << env::SYSTEM_COLOR << "In file: " << env::RESET_COLOR << normalizedFileName << std::endl;                                   // File name
    std::cerr << env::SYSTEM_COLOR << "In line: " << env::RESET_COLOR << lineNumber << std::endl;                                           // Line number
    std::cerr << env::SYSTEM_COLOR << "--> " << env::ERROR_COLOR << errorMessage << env::RESET_COLOR << std::endl;                          // Error message
    std::cerr << env::SYSTEM_COLOR << "\nAborting..." << env::RESET_COLOR << std::endl;                                                     // Abort notice
    std::cerr << "\n" << std::endl;                                                                                                         // Final spacing
    std::exit(1);                                                                                                                           // Terminate the program
}

// PrintWarning function ------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print a formatted warning message.
 *
 * @param functionName Name of the function reporting the warning.
 * @param fileName Source file where the warning occurred.
 * @param lineNumber Source line where the warning occurred.
 * @param warningMessage Human-readable warning description.
 *
 * Unlike `ExitWithError`, this function only reports the issue and returns.
 */
inline void PrintWarning(const std::string& functionName, const std::string& fileName, int lineNumber, const std::string& warningMessage) {
    const std::string normalizedFileName = NormalizeFilePathForPrint(fileName);
    std::cerr << "\n" << std::endl;                                                                                                             // Separate warning visually with a blank line
    std::cerr << env::SYSTEM_COLOR << "[" << functionName << "]" << ": " << env::WARNING_COLOR << "Warning!" << env::RESET_COLOR << std::endl;  // Header
    std::cerr << env::SYSTEM_COLOR << "In file: " << env::RESET_COLOR << normalizedFileName << std::endl;                                       // File name
    std::cerr << env::SYSTEM_COLOR << "In line: " << env::RESET_COLOR << lineNumber << std::endl;                                               // Line number
    std::cerr << env::SYSTEM_COLOR << "--> " << env::WARNING_COLOR << warningMessage << env::RESET_COLOR << std::endl;                          // Warning message
    std::cerr << "\n" << std::endl;                                                                                                             // Final spacing
}

// ======================================================================================================================================================================
// Git information functions
// ======================================================================================================================================================================

// QuoteForShell function -----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Quote a string for safe single-quoted shell usage.
 *
 * @param raw Raw string to quote.
 * @return Single-quoted shell-safe string.
 */
inline std::string QuoteForShell(const std::string& raw) {
    std::string quoted = "'";
    for (const char c : raw) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
}

// RunShellCommand function ---------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Execute a shell command and return trimmed stdout.
 *
 * @param command Command string to execute.
 * @param suppress_stderr When true, redirect stderr to `/dev/null`.
 * @return Captured and trimmed stdout; empty string on failure.
 */
inline std::string RunShellCommand(const std::string& command, const bool suppress_stderr = true) {
    const std::string full_command = suppress_stderr ? (command + " 2>/dev/null") : command;
    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) { return ""; }

    std::array<char, 256> buffer{};
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) { output += buffer.data(); }
    pclose(pipe);
    return TrimWhitespace(output);
}

// GitHubRepoUrlFromRemote function ------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Normalize a GitHub remote URL to HTTPS repository URL format.
 *
 * @param remote_url Remote URL such as `git@github.com:org/repo.git`.
 * @return Normalized URL such as `https://github.com/org/repo`, or empty string for unsupported hosts.
 */
inline std::string GitHubRepoUrlFromRemote(const std::string& remote_url) {
    std::string normalized = remote_url;
    if (normalized.empty()) { return ""; }

    if (normalized.rfind("git@github.com:", 0) == 0) {
        normalized = "https://github.com/" + normalized.substr(std::string("git@github.com:").size());
    } else if (normalized.rfind("http://github.com/", 0) == 0) {
        normalized = "https://github.com/" + normalized.substr(std::string("http://github.com/").size());
    } else if (normalized.rfind("https://github.com/", 0) != 0) {
        return "";
    }

    if (normalized.size() >= 4 && normalized.substr(normalized.size() - 4) == ".git") { normalized.erase(normalized.size() - 4); }
    while (!normalized.empty() && normalized.back() == '/') { normalized.pop_back(); }
    return normalized;
}

// ======================================================================================================================================================================
// General functions
// ======================================================================================================================================================================

// quit function --------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Exit the program with a failure status.
 *
 * Thin shorthand wrapper around `std::exit(1)`.
 */
inline void quit() {
    std::exit(1);  // Exit with failure status
}

// BoolToChar function --------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Convert a boolean to a string literal.
 *
 * @param b Boolean value to convert.
 * @return `"true"` when `b` is true, otherwise `"false"`.
 */
inline const char* const BoolToChar(bool b) {
    return b ? "true" : "false";  // Return string literal for boolean value
}

// BoolToString function ------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Convert a boolean to an owning `std::string`.
 *
 * @param b Boolean value to convert.
 * @return `"true"` when `b` is true, otherwise `"false"`.
 */
inline std::string BoolToString(bool b) {
    return b ? "true" : "false";  // Return std::string for boolean value
}

// ToStringWithPrecision function ---------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Convert a numeric-like value to a fixed-point string with controlled precision.
 *
 * @tparam A Value type accepted by the output stream.
 * @param a_value Value to format.
 * @param n Number of digits after the decimal point.
 * @return Formatted string.
 */
template <typename A>
std::string ToStringWithPrecision(const A a_value, const int n = 2) {
    std::ostringstream out;        // Create output string stream
    out.precision(n);              // Set precision for fractional digits
    out << std::fixed << a_value;  // Print value in fixed-point notation
    return out.str();              // Return formatted string
}

// FindSubstring function -----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Check whether one string appears inside another.
 *
 * @param string1 String to search in.
 * @param string2 Substring to search for.
 * @return `true` if `string2` occurs anywhere inside `string1`.
 */
inline bool FindSubstring(const std::string& string1, const std::string& string2) {
    return string1.find(string2) != std::string::npos;  // Return true if substring found
}

// ReplaceSubstring function --------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Replace the first occurrence of one substring with another.
 *
 * @param input Original input string.
 * @param toReplace Substring to search for.
 * @param replaceWith Replacement text.
 * @return Updated string if a match is found, otherwise the original string.
 */
inline std::string ReplaceSubstring(const std::string& input, const std::string& toReplace, const std::string& replaceWith) {
    size_t pos = input.find(toReplace);  // Find first occurrence
    if (pos == std::string::npos) {
        return input;  // If not found, return original string
    }
    // Replace first occurrence
    return input.substr(0, pos) + replaceWith + input.substr(pos + toReplace.length());
}

// trim function --------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Remove leading and trailing spaces or tabs from a string.
 *
 * @param str Input string.
 * @return Trimmed copy.
 */
inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");  // Find first non-space/tab
    if (first == std::string::npos) return "";    // All spaces: return empty string
    size_t last = str.find_last_not_of(" \t");    // Find last non-space/tab
    return str.substr(first, last - first + 1);   // Return trimmed substring
}

// TrimWhitespace function ----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Remove leading/trailing spaces, tabs, and line breaks from a string.
 *
 * @param value Input string.
 * @return Trimmed copy.
 */
inline std::string TrimWhitespace(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) { value.pop_back(); }
    std::size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\n' || value[start] == '\r')) { ++start; }
    if (start > 0) { value.erase(0, start); }
    return value;
}

// splitVarAndUnits function --------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Split a label of the form `name [units]` into name and units parts.
 *
 * @param input Input label string.
 * @return Pair of `{variable, units}` where the units element is empty if no `[` is found.
 */
inline std::pair<std::string, std::string> splitVarAndUnits(const std::string& input) {
    size_t bracketPos = input.find('[');  // Find start of units
    if (bracketPos == std::string::npos) {
        return {trim(input), ""};  // No units found, return label only
    }
    std::string var = input.substr(0, bracketPos);  // Extract variable name
    std::string units = input.substr(bracketPos);   // Extract units (including bracket)
    return {trim(var), trim(units)};                // Return trimmed pair
}

// GetBeamEnergyFromString function -------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Infer the beam energy value from a sample-name-like string.
 *
 * @param sn Source string to inspect.
 * @return 5.98636, 4.02962, 2.07052, or 0.0 when no supported pattern is found.
 *
 * The function recognizes both exact MeV-style labels and shorter GeV-style aliases.
 */
inline double GetBeamEnergyFromString(const std::string& sn) {
    double BeamE_double = 0.;
    // Check for 5.98636 GeV patterns
    if (FindSubstring(sn, "598636MeV") || FindSubstring(sn, "598636mev") || FindSubstring(sn, "598636") || FindSubstring(sn, "5986MeV") || FindSubstring(sn, "5986mev") ||
        FindSubstring(sn, "5986") || FindSubstring(sn, "6GeV") || FindSubstring(sn, "6Gev") || FindSubstring(sn, "6gev")) {
        BeamE_double = 5.98636;
    }
    // Check for 4.02962 GeV patterns
    else if (FindSubstring(sn, "402962MeV") || FindSubstring(sn, "402962mev") || FindSubstring(sn, "402962") || FindSubstring(sn, "4029MeV") || FindSubstring(sn, "4029mev") ||
             FindSubstring(sn, "4029") || FindSubstring(sn, "4GeV") || FindSubstring(sn, "4Gev") || FindSubstring(sn, "4gev")) {
        BeamE_double = 4.02962;
    }
    // Check for 2.07052 GeV patterns
    else if (FindSubstring(sn, "207052MeV") || FindSubstring(sn, "207052mev") || FindSubstring(sn, "207052") || FindSubstring(sn, "2070MeV") || FindSubstring(sn, "2070mev") ||
             FindSubstring(sn, "2070") || FindSubstring(sn, "2GeV") || FindSubstring(sn, "2Gev") || FindSubstring(sn, "2gev")) {
        BeamE_double = 2.07052;
    }
    // Return 0.0 if nothing matched
    return BeamE_double;
}

// GetBeamEnergyFromDouble function -------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Convert a beam-energy value into the framework's canonical MeV-style label.
 *
 * @param BeamE Beam energy value in GeV.
 * @return `5986MeV`, `4029MeV`, `2070MeV`, or an empty string when the rounded value
 *         does not match a supported beam setting.
 */
inline std::string GetBeamEnergyFromDouble(const double& BeamE) {
    std::string BeamE_str;
    // Map rounded values to canonical string labels
    if (std::round(BeamE) == 6) {
        BeamE_str = "5986MeV";
    } else if (std::round(BeamE) == 4) {
        BeamE_str = "4029MeV";
    } else if (std::round(BeamE) == 2) {
        BeamE_str = "2070MeV";
    }
    // Return empty string if no match
    return BeamE_str;
}

/**
 * @brief Check whether a file can be opened for reading.
 *
 * @param filename Path to test.
 * @return `true` when the file stream opens successfully.
 */
inline bool fileExists(const char* filename) {
    std::ifstream file(filename);  // Attempt to open file
    return file.good();            // Return true if opened successfully
}

/**
 * @brief Extract the remote IP address from the `SSH_CONNECTION` environment string.
 *
 * @param ssh_connection Raw `SSH_CONNECTION` environment variable.
 * @return First whitespace-delimited field, or an empty string when unavailable.
 */
inline std::string GetRemoteIPFromSSHConnection(const char* ssh_connection) {
    if (ssh_connection == nullptr) { return ""; }  // Return empty if not available
    std::istringstream sshConnectionStream(ssh_connection);
    std::string remote_ip;
    sshConnectionStream >> remote_ip;  // Take the first field as the remote IP
    return remote_ip;
}

/**
 * @brief Resolve a host name from a numeric IP address.
 *
 * @param remote_ip Numeric IP address string.
 * @param host_name Output string that receives the resolved host name on success.
 * @return `true` on successful reverse lookup, otherwise `false`.
 */
inline bool ResolveHostNameFromIP(const std::string& remote_ip, std::string& host_name) {
    if (remote_ip.empty()) { return false; }  // Can't resolve empty IP
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST;
    addrinfo* result = nullptr;
    const int gaiStatus = getaddrinfo(remote_ip.c_str(), nullptr, &hints, &result);
    if (gaiStatus != 0 || result == nullptr) { return false; }  // IP not resolvable
    char hostBuffer[NI_MAXHOST];
    const int nameInfoStatus = getnameinfo(result->ai_addr, result->ai_addrlen, hostBuffer, sizeof(hostBuffer), nullptr, 0, NI_NAMEREQD);
    freeaddrinfo(result);
    if (nameInfoStatus != 0) { return false; }  // Reverse lookup failed
    host_name = hostBuffer;                     // Store resolved host name
    return true;
}

// CheckSSHConnection function ------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print whether the current process appears to run inside an SSH session.
 *
 * If `SSH_CONNECTION` exists, the full variable content is printed.
 */
inline void CheckSSHConnection() {
    const char* ssh_connection = std::getenv("SSH_CONNECTION");
    if (ssh_connection) {
        std::cout << env::SYSTEM_COLOR << "Connected via SSH. SSH_CONNECTION: " << env::RESET_COLOR << ssh_connection << std::endl << std::flush;
    } else {
        std::cout << env::SYSTEM_COLOR << "Not connected via SSH." << env::RESET_COLOR << std::endl << std::flush;
    }
}

// CheckSSHConnectionAndHost function -----------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Print SSH connection information and attempt reverse host-name resolution.
 *
 * The function reports the raw `SSH_CONNECTION` string, the parsed remote IP, and,
 * when possible, the resolved remote host name.
 */
inline void CheckSSHConnectionAndHost() {
    const char* ssh_connection = std::getenv("SSH_CONNECTION");
    if (ssh_connection) {
        std::cout << env::SYSTEM_COLOR << "SSH_CONNECTION: " << env::RESET_COLOR << ssh_connection << std::endl << std::flush;
        const std::string remote_ip = GetRemoteIPFromSSHConnection(ssh_connection);  // Parse remote IP
        if (remote_ip.empty()) {
            std::cerr << env::SYSTEM_COLOR << "Could not parse remote IP from SSH_CONNECTION." << env::RESET_COLOR << std::endl << std::flush;
            return;
        }
        std::cout << env::SYSTEM_COLOR << "Remote IP Address: " << env::RESET_COLOR << remote_ip << std::endl << std::flush;
        std::string host_name;
        if (ResolveHostNameFromIP(remote_ip, host_name)) {
            std::cout << env::SYSTEM_COLOR << "Remote Host Name: " << env::RESET_COLOR << host_name << std::endl << std::flush;
        } else {
            std::cerr << env::SYSTEM_COLOR << "Could not resolve host name from IP." << env::RESET_COLOR << std::endl << std::flush;
        }
    } else {
        std::cout << env::SYSTEM_COLOR << "Not connected via SSH." << env::RESET_COLOR << std::endl << std::flush;
    }
}

// GetSSHHostName function ----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Return the resolved remote host name for the current SSH session.
 *
 * @return Resolved host name, or an empty string if not connected through SSH or if
 *         reverse lookup fails.
 */
inline std::string GetSSHHostName() {
    const char* ssh_connection = std::getenv("SSH_CONNECTION");
    if (ssh_connection) {
        const std::string remote_ip = GetRemoteIPFromSSHConnection(ssh_connection);  // Parse remote IP
        if (remote_ip.empty()) {
            std::cerr << env::ERROR_COLOR << "Error! " << env::RESET_COLOR << "Could not parse remote IP from SSH_CONNECTION." << std::endl << std::flush;
            return "";
        }
        std::string host_name;
        if (ResolveHostNameFromIP(remote_ip, host_name)) { return host_name; }  // Return host name if resolved
        std::cerr << env::ERROR_COLOR << "Error! " << env::RESET_COLOR << "Could not resolve host name from IP." << std::endl << std::flush;
        return "";
    }
    std::cerr << env::ERROR_COLOR << "Error! " << env::RESET_COLOR << "Not connected via SSH." << std::endl << std::flush;
    return "";
}

// GetCurrentDirectory function -----------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Return the current working directory.
 *
 * @return Current directory path, or an empty string if `getcwd` fails.
 */
inline std::string GetCurrentDirectory() {
    char pwd[PATH_MAX];
    if (getcwd(pwd, sizeof(pwd)) == nullptr) {
        std::cerr << env::ERROR_COLOR << "Error! " << env::RESET_COLOR << "Error getting current directory." << std::endl << std::endl << std::flush;
        return "";  // Return empty string on failure
    }
    return std::string(pwd);  // Return current directory as string
}

// ======================================================================================================================================================================
// Hipo files functions
// ======================================================================================================================================================================

// LogSkippedHipoFiles function -----------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Write the list of skipped HIPO files to a text report.
 *
 * @param SkippedHipoChainFiles Vector of skipped file names.
 * @param HipoChainLength Total number of files considered for the chain.
 * @param filename Output text-file path.
 *
 * The report includes a header, chain statistics, and the full skipped-file list.
 */
inline void LogSkippedHipoFiles(const std::vector<TString>& SkippedHipoChainFiles, int HipoChainLength, const char* filename) {
    std::ofstream outFile(filename);  // Open output file
    if (outFile.is_open()) {
        outFile << "\n===========================================================================================\n";
        outFile << "Skipped HIPO files\n";
        outFile << "===========================================================================================\n\n";
        outFile << "Total number of files in chain: " << HipoChainLength << "\n";
        outFile << "Number of skipped files: " << SkippedHipoChainFiles.size() << "\n\n";
        outFile << "-- Skipped files list ---------------------------------------------------------------------\n\n";
        for (const auto& fname : SkippedHipoChainFiles) { outFile << fname << std::endl; }  // Write each skipped file
        outFile << "\n";
        outFile.close();  // Close the output file
        std::cout << env::COMPLETION_COLOR << "Skipped file list saved to: \n" << env::RESET_COLOR << filename << std::endl << std::flush;
    } else {
        ExitWithError(__func__, __FILE__, __LINE__, std::string("Failed to open file:\n") + filename + " for writing!");  // Report error if cannot open
    }
}

};  // namespace basic_tools

#endif  // BASIC_TOOLS_H
