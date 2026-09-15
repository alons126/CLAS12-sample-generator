//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.cpp
 * @brief Sample option parsing, validation and JSON escaping.
 *
 * Purpose:
 *   Keep invalid settings out of the generation loop and record resolved configuration.
 *
 * Workflow:
 *   Defaults -> profile -> CLI overrides -> automatic values -> validation -> absolute paths.
 */

#include "common/RunConfig.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "common/TargetGeometry.h"
namespace samples {
namespace {
// trim ----------------------------------------------------------------------

#pragma region /* trim */
/**
 * @brief Remove surrounding configuration whitespace.
 *
 * Algorithm:
 *   Find the first and last non-whitespace characters; retain the enclosed text.
 *
 * @param s Text from a configuration line.
 *
 * @return Trimmed text, or an empty string for whitespace-only input.
 */
std::string trim(std::string s) {
    auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
#pragma endregion

}  // namespace

// RunConfig::parse ----------------------------------------------------------------------

#pragma region /* RunConfig::parse */
/**
 * @brief Resolve one complete sample configuration.
 *
 * Purpose:
 *   Make both executables use the same precedence and validation contract before creating outputs.
 *
 * Algorithm:
 *   1. Install defaults and collect CLI overrides.
 *   2. Read one optional profile, rejecting unknown or repeated keys.
 *   3. Apply overrides and resolve channel-dependent automatic settings.
 *   4. Validate values and normalize local input/output paths.
 *
 * @param argc Number of CLI tokens.
 * @param argv CLI tokens including executable name.
 * @param genie True for GST conversion, false for uniform generation.
 *
 * @return Validated settings; throws on malformed options or invalid physical bounds.
 */
RunConfig RunConfig::parse(int argc, char** argv, bool genie) {
#pragma region /* Default settings */
    // Install shared defaults before workflow-specific options.
    RunConfig c;
    c.values_ = {{"beam-energy", "5.98636"},
                 {"target", "Ar"},
                 {"A", "1"},
                 {"Z", "1"},
                 {"output", ""},
                 {"files", "1"},
                 {"events-per-file", "10000"},
                 {"seed", "67890"},
                 {"lund-format", "legacy"},
                 {"mass-convention", "legacy"},
                 {"render-plots", "false"},
                 {"vertex-seed", "12345"},
                 {"prefix", genie ? "GENIE_sample" : "Uniform_sample"}};

    if (genie) {
        c.values_.insert({{"input", ""}});
    } else {
        c.values_.insert({{"channel", "1e"},
                          {"electron-theta-min", "5"},
                          {"electron-theta-max", "40"},
                          {"nucleon-theta-min", "5"},
                          {"nucleon-theta-max", "auto"},
                          {"nucleon-momentum", "fixed"},
                          {"nucleon-angle", "auto"},
                          {"nucleon-p", "1"},
                          {"nucleon-p-min", "0.3"},
                          {"nucleon-p-max", "auto"},
                          {"electron-momentum", "uniform"},
                          {"trigger-theta", "25"},
                          {"trigger-phi-offset", "auto"}});
    }

    auto assign = [&](const std::string& k, const std::string& v) {
        if (!c.values_.count(k)) { throw std::runtime_error("Unknown setting: " + k); }

        c.values_[k] = v;
    };
#pragma endregion

#pragma region /* Profile and CLI input */
    // Collect CLI overrides separately so their precedence is independent of argument order.
    std::map<std::string, std::string> overrides;
    std::string config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--", 0) != 0 || i + 1 == argc) { throw std::runtime_error("Expected --key value: " + arg); }

        auto key = arg.substr(2);
        std::string value = argv[++i];

        if (key == "config") {
            if (!config.empty()) { throw std::runtime_error("Use only one --config file"); }

            config = value;
        } else {
            if (!overrides.emplace(key, value).second) { throw std::runtime_error("Repeated option: " + key); }
        }
    }

    // Read one profile, rejecting duplicate keys instead of silently replacing values.
    if (!config.empty()) {
        std::ifstream in(config);

        if (!in) { throw std::runtime_error("Cannot open config: " + config); }

        std::string line;
        std::map<std::string, bool> seen;

        while (std::getline(in, line)) {
            line = trim(line);

            if (line.empty() || line[0] == '#') continue;

            auto eq = line.find('=');

            if (eq == std::string::npos) { throw std::runtime_error("Expected key = value: " + line); }

            auto key = trim(line.substr(0, eq));

            if (seen[key]) { throw std::runtime_error("Repeated config key: " + key); }

            seen[key] = true;

            assign(key, trim(line.substr(eq + 1)));
        }
    }
#pragma endregion

    // Apply explicit options last, then resolve channel and beam dependent defaults.
    for (const auto& [k, v] : overrides) assign(k, v);

    if (!genie) {
        if (c.get("nucleon-theta-max") == "auto") c.values_["nucleon-theta-max"] = c.get("channel") == "en" ? "35" : "45";
        if (c.get("nucleon-p-max") == "auto") c.values_["nucleon-p-max"] = c.get("beam-energy");
        if (c.get("trigger-phi-offset") == "auto") {
            double e = c.number("beam-energy");
            c.values_["trigger-phi-offset"] = std::abs(e - 2.07052) < 1e-6 ? "16" : std::abs(e - 4.02962) < 1e-6 ? "7" : std::abs(e - 5.98636) < 1e-6 ? "5" : "0";
        }
    }

    if (!genie) {
        if (c.get("nucleon-momentum") == "sampled") c.values_["nucleon-momentum"] = c.get("channel") == "ep" ? "mixed" : "uniform";
        if (c.get("nucleon-angle") == "auto") c.values_["nucleon-angle"] = c.get("channel") == "en" && c.get("nucleon-momentum") != "fixed" ? "isotropic" : "theta";
    }

    // Validate before normalizing paths or allowing downstream output creation.
    c.validate(genie);

    if (genie && c.get("input").find("://") == std::string::npos) c.values_["input"] = std::filesystem::absolute(c.get("input")).lexically_normal().string();

    c.values_["output"] = std::filesystem::absolute(c.get("output")).lexically_normal().string();

    return c;
}
#pragma endregion

// RunConfig::get ----------------------------------------------------------------------

#pragma region /* RunConfig::get */
/**
 * @brief Read a resolved setting as text.
 *
 * Algorithm:
 *   Look up the key in the validated configuration map.
 *
 * @param k Known configuration key.
 *
 * @return Stored string; a missing key throws.
 */
std::string RunConfig::get(const std::string& k) const { return values_.at(k); }
#pragma endregion

// RunConfig::number ----------------------------------------------------------------------

#pragma region /* RunConfig::number */
/**
 * @brief Read a finite floating-point setting.
 *
 * Algorithm:
 *   Convert the string and reject trailing characters or non-finite values.
 *
 * @param k Numeric configuration key.
 *
 * @return Finite double; throws on invalid numeric input.
 */
double RunConfig::number(const std::string& k) const {
    std::size_t used = 0;
    double value = std::stod(get(k), &used);

    if (used != get(k).size() || !std::isfinite(value)) { throw std::runtime_error("Invalid number: " + k); }

    return value;
}
#pragma endregion

// RunConfig::integer ----------------------------------------------------------------------

#pragma region /* RunConfig::integer */
/**
 * @brief Read an unsigned integer setting.
 *
 * Algorithm:
 *   Require decimal digits only, then convert with overflow checking.
 *
 * @param k Integer configuration key.
 *
 * @return Unsigned value; throws on malformed or out-of-range input.
 */
std::uint64_t RunConfig::integer(const std::string& k) const {
    const auto s = get(k);

    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) { throw std::runtime_error("Expected unsigned integer: " + k); }

    return std::stoull(s);
}
#pragma endregion

// RunConfig::validate ----------------------------------------------------------------------

#pragma region /* RunConfig::validate */
/**
 * @brief Reject incompatible or invalid run settings before output creation.
 *
 * Algorithm:
 *   1. Check output, counts, seeds, metadata and shared output options.
 *   2. Validate the target against the external geometry map.
 *   3. Require GST input or check channel-specific momentum and angular bounds.
 *
 * @param genie Select conversion-specific validation when true.
 *
 * @note No value; throws with the invalid setting or constraint.
 */
void RunConfig::validate(bool genie) const {
    if (get("output").empty()) { throw std::runtime_error("--output is required; use a new run directory"); }
    if (number("beam-energy") <= 0) { throw std::runtime_error("beam-energy must be positive"); }
    for (auto k : {"files", "events-per-file", "seed", "vertex-seed"}) {
        auto n = integer(k);
        if (!n || n > std::numeric_limits<unsigned int>::max()) { throw std::runtime_error(std::string(k) + " must be in [1, 4294967295]"); }
    }
    if (integer("A") < 1 || integer("A") > 300 || integer("Z") > integer("A")) { throw std::runtime_error("Require 1 <= A <= 300 and 0 <= Z <= A"); }
    if (get("prefix").empty() || get("prefix").find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos) {
        throw std::runtime_error("prefix must contain only letters, numbers, _, . or -");
    }
    if (get("lund-format") != "legacy" && get("lund-format") != "precise") { throw std::runtime_error("lund-format must be legacy or precise"); }
    if (get("mass-convention") != "legacy" && get("mass-convention") != "standard") { throw std::runtime_error("mass-convention must be legacy or standard"); }
    if (get("render-plots") != "true" && get("render-plots") != "false") { throw std::runtime_error("render-plots must be true or false"); }

    TargetGeometry::validate(get("target"));

    if (genie) {
        if (get("input").empty()) { throw std::runtime_error("--input GST ROOT file or glob is required"); }
        return;
    }

    if (get("channel") != "1e" && get("channel") != "ep" && get("channel") != "en") { throw std::runtime_error("channel must be 1e, ep or en"); }

    for (auto stem : {"electron", "nucleon"}) {
        double lo = number(std::string(stem) + "-theta-min"), hi = number(std::string(stem) + "-theta-max");
        if (!(0 <= lo && lo < hi && hi <= 180)) { throw std::runtime_error("Require 0 <= theta-min < theta-max <= 180"); }
    }

    if (get("nucleon-momentum") != "fixed" && get("nucleon-momentum") != "uniform" && get("nucleon-momentum") != "mixed") {
        throw std::runtime_error("nucleon-momentum must be fixed, sampled, uniform or mixed");
    }
    if (get("nucleon-momentum") == "mixed" && (get("channel") != "ep" || number("nucleon-p-min") <= 0)) { throw std::runtime_error("mixed requires ep and strictly positive nucleon-p-min"); }
    if (get("nucleon-angle") != "theta" && get("nucleon-angle") != "isotropic") { throw std::runtime_error("nucleon-angle must be auto, theta or isotropic"); }
    if (get("electron-momentum") != "uniform" && get("electron-momentum") != "beam") { throw std::runtime_error("electron-momentum must be uniform or beam"); }
    if (number("nucleon-p") <= 0 || number("nucleon-p-min") < 0 || number("nucleon-p-max") <= number("nucleon-p-min")) { throw std::runtime_error("Invalid nucleon momentum bounds"); }
    if (number("trigger-theta") < 0 || number("trigger-theta") > 180 || std::abs(number("trigger-phi-offset")) > 180) { throw std::runtime_error("Invalid trigger angle"); }
}
#pragma endregion

// jsonString ----------------------------------------------------------------------

#pragma region /* jsonString */
/**
 * @brief Encode a string as a JSON string literal.
 *
 * Algorithm:
 *   Escape quotes, backslashes and control characters, then enclose in quotes.
 *
 * @param s Unescaped configuration or provenance text.
 *
 * @return JSON-safe text including surrounding quotes.
 */
std::string jsonString(const std::string& s) {
    std::ostringstream out;
    out << '"';

    for (unsigned char ch : s) {
        if (ch == '"' || ch == '\\')
            out << '\\' << ch;
        else if (ch < 0x20)
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
        else
            out << ch;
    }

    out << '"';

    return out.str();
}
#pragma endregion

// help ----------------------------------------------------------------------

#pragma region /* help */
/**
 * @brief Describe CLI options for the selected application.
 *
 * Algorithm:
 *   Combine shared option descriptions with workflow-specific usage.
 *
 * @param genie True for the conversion help page.
 *
 * @return Usage text; does not print or execute a workflow.
 */
std::string help(bool genie) {
    std::string result = genie ? "clas12-genie-to-lund --input 'gst*.root' --output NEW_DIRECTORY\n" : "clas12-uniform --channel 1e|ep|en --output NEW_DIRECTORY\n";
    result +=
        "Settings: --config FILE, --beam-energy GeV, --target GEOMETRY, --A N, --Z N,\n"
        "--files N, --events-per-file N, --seed N, --vertex-seed N, --prefix NAME,\n"
        "--lund-format legacy|precise, --mass-convention legacy|standard, --render-plots true|false.\n"
        "Files use key = value; CLI values override file settings. No automatic overwrite.\n";

    if (!genie) {
        result +=
            "Uniform: --electron-theta-min/max DEG, --nucleon-theta-min/max DEG,\n"
            "--electron-momentum uniform|beam, --nucleon-momentum fixed|sampled|uniform|mixed,\n"
            "--nucleon-angle auto|theta|isotropic, --nucleon-p GeV, --nucleon-p-min/max GeV, --trigger-theta DEG, --trigger-phi-offset DEG.\n";
    }

    return result;
}
#pragma endregion

}  // namespace samples
