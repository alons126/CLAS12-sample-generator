//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.cpp
 * @brief Builds and checks final LUND run settings.
 *
 * Purpose:
 *   Build one valid configuration for uniform generation or physical conversion before either workflow
 *   starts processing events. Keep the exact final values so the manifest can record them.
 *
 * Workflow:
 *   Defaults -> profile -> CLI overrides -> automatic values -> validation -> absolute paths.
 *
 * Accepted options:
 *   Shared: config, beam-energy, rgm-target, target, A, Z, output, events, events-per-file,
 *           seed, vertex-seed, prefix, and gemc-target-variation.
 *   Uniform: channel, hadron, hadron-region, electron-theta-min/max, electron-p-min/max,
 *            electron-momentum, hadron-theta-min/max, hadron-p-min, hadron-p,
 *            hadron-momentum, trigger-theta, and trigger-phi-offset.
 *   Physical: input, event-generator, event-generator-version, tune, q2-cut, and gemc-version.
 *   Each option uses `--key value`. The application handles `--help` before parse(). A profile uses the
 *   same names without `--` and writes them as `key = value`.
 *
 * Setting priority:
 *   Command-line values replace profile values, and profile values replace defaults. The code resolves
 *   `auto` only after all explicit values are known. Each application documents its defaults and units.
 *
 * Boundary:
 *   This file only prepares settings. It does not create or convert events, read GST trees, draw random
 *   values, replace output directories, write LUND records, create monitoring, or submit jobs.
 */

#include "core/config/RunConfig.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "core/config/RgmTarget.h"
#include "core/geometry/TargetGeometry.h"

namespace samples {

// Translation-unit helpers ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Translation-unit helpers */
/**
 * @namespace samples::<anonymous>
 * @brief Text and naming helpers used only in this file.
 *
 * These functions read profile text and build output names. They are not part of the public API.
 */
namespace {

// trim ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* trim */
/**
 * @brief Remove spaces around configuration text.
 *
 * Purpose:
 *   Remove whitespace around profile lines, keys, and values without changing spaces inside the text.
 *
 * Steps:
 *   Find the first and last non-space characters and return the text between them. Return an empty
 *   string when no text remains.
 *
 * @param s Copy of the configuration text to trim.
 *
 * @return Text without leading/trailing spaces, tabs, carriage returns, or newlines; empty for an
 *         empty or whitespace-only input.
 *
 * @note Spaces inside the value and all other characters stay unchanged.
 */
std::string trim(std::string s) {
    // No first character means the value is empty or only spaces.
    auto first = s.find_first_not_of(" \t\r\n");

    // Use the last non-space character as the other end.
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
#pragma endregion

// pathToken -------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* pathToken */
/**
 * @brief Make one setting safe to use inside an output-directory name.
 *
 * Purpose:
 *   Keep target, generator, tune, cut, and GEMC text readable without allowing that text to create extra
 *   path components.
 *
 * Steps:
 *   Keep letters, digits, `.`, `_`, and `-`. Replace every other character with `-`. Reject an empty
 *   result and the special path names `.` and `..`.
 *
 * @param value Final setting text copied from the configuration.
 *
 * @return Text that is safe to place in one output-directory component.
 *
 * @throws std::runtime_error If the resulting component is empty, `.` or `..`.
 *
 * @note Different inputs can produce the same safe text. The run log keeps the original value.
 */
std::string pathToken(std::string value) {
    // std::isalnum needs an unsigned byte. Keep only the three allowed punctuation marks.
    for (char& ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '_' || ch == '-')) { ch = '-'; }
    }

    // Reject empty names and the special `.` and `..` paths.
    if (value.empty() || value == "." || value == "..") { throw std::runtime_error("Invalid empty output-name component"); }
    return value;
}
#pragma endregion

// beamMeV ---------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* beamMeV */
/**
 * @brief Convert beam energy in GeV to the integer MeV label used in names.
 *
 * Purpose:
 *   Keep the established 2070, 4029, and 5986 MeV labels and provide a rounded label for other energies.
 *
 * Steps:
 *   Return the fixed label when the energy is within `1e-6` GeV of a known setting. Otherwise multiply
 *   by 1000 and round to the nearest integer.
 *
 * @param energy Finite beam energy in GeV.
 *
 * @return Beam-energy label in MeV for filenames and directory names.
 *
 * @note 2.07052 GeV intentionally uses `2070`, not the normally rounded value `2071`.
 */
long long beamMeV(double energy) {
    if (std::abs(energy - 2.07052) < 1e-6) { return 2070; }
    if (std::abs(energy - 4.02962) < 1e-6) { return 4029; }
    if (std::abs(energy - 5.98636) < 1e-6) { return 5986; }
    return std::llround(energy * 1000.0);
}
#pragma endregion

// uniformSampleLabel ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* uniformSampleLabel */
/**
 * @brief Build the uniform-sample label from its particle and detector settings.
 * @param config Resolved configuration containing channel, hadron, and hadron-region.
 * @return `1e`, `electron-tester`, or one of the resolved electron-hadron FD/CD labels.
 */
std::string uniformSampleLabel(const RunConfig& config) {
    if (config.get("channel") == "1e") { return "1e"; }
    if (config.get("channel") == "electron-tester") { return "electron-tester"; }

    const auto& hadron = config.get("hadron");
    const std::string token = hadron == "proton" ? "p" : hadron == "neutron" ? "n" : hadron;
    return "e" + token + config.get("hadron-region");
}
#pragma endregion

}  // namespace
#pragma endregion

// RunConfig::parse ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::parse */
RunConfig RunConfig::parse(int argc, char** argv, bool uniform) {
#pragma region /* Default settings */
    // Keep settings as text so the run log records their exact final values. These defaults select the
    // usual RG-M beam and argon target and keep separate seeds for kinematics and vertices. Particle
    // masses come from targets.h and cannot be set here.
    RunConfig c;
    c.values_ = {{"beam-energy", "5.98636"},
                 {"rgm-target", "Ar40"},
                 {"target", "auto"},
                 {"A", "auto"},
                 {"Z", "auto"},
                 {"gemc-target-variation", "auto"},
                 {"output", ""},
                 {"events", ""},
                 {"events-per-file", uniform ? "25000" : "10000"},
                 {"seed", "67890"},
                 {"vertex-seed", "12345"},
                 {"prefix", "auto"}};

    // Add only settings used by the selected source. An option for the other source is rejected instead
    // of being accepted and ignored.
    if (uniform) {
        // Angles use degrees and momenta use GeV/c. Resolve `auto` after reading the profile and command
        // line. Fixed 1 GeV/c momentum is available only for neutrons.
        c.values_.insert({{"channel", "1e"},
                          {"hadron", "proton"},
                          {"hadron-region", "FD"},
                          {"electron-theta-min", "5"},
                          {"electron-theta-max", "40"},
                          {"electron-p-min", "0.7"},
                          {"electron-p-max", "auto"},
                          {"hadron-theta-min", "auto"},
                          {"hadron-theta-max", "auto"},
                          {"hadron-momentum", "auto"},
                          {"hadron-p", "1"},
                          {"hadron-p-min", "auto"},
                          {"electron-momentum", "auto"},
                          {"trigger-theta", "25"},
                          {"trigger-phi-offset", "auto"}});
    } else {
        // Physical mode reads existing event-generator truth and records where it came from. It does not
        // run GENIE. The current adapter reads GENIE GST input.
        c.values_.insert({{"input", ""}, {"event-generator", "genie-gst"}, {"event-generator-version", "unknown"}, {"tune", "unknown"}, {"q2-cut", "auto"}, {"gemc-version", "unknown"}});
    }

    // Use the same assignment check for profiles and command-line values.
    auto assign = [&](const std::string& k, const std::string& v) {
        if (!c.values_.count(k)) { throw std::runtime_error("Unknown setting: " + k); }

        c.values_[k] = v;
    };
#pragma endregion

#pragma region /* Profile and CLI input */
    // Save command-line values until the profile is read. The map also detects repeated options.
    std::map<std::string, std::string> overrides;
    std::string config;

    // Every option is a `--key value` pair. Negative numbers are read as values, not new options.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--", 0) != 0 || i + 1 == argc) { throw std::runtime_error("Expected --key value: " + arg); }

        auto key = arg.substr(2);
        std::string value = argv[++i];

        // `config` names the profile file. It is not stored as a run setting.
        if (key == "config") {
            if (!config.empty()) { throw std::runtime_error("Use only one --config file"); }

            config = value;
        } else {
            if (!overrides.emplace(key, value).second) { throw std::runtime_error("Repeated option: " + key); }
        }
    }

    // Read at most one profile and reject repeated keys. Relative profile paths start from the current
    // working directory; workflow.py uses the repository root for maintained launches.
    if (!config.empty()) {
        std::ifstream in(config);

        if (!in) { throw std::runtime_error("Cannot open config: " + config); }

        std::string line;
        std::map<std::string, bool> seen;

        while (std::getline(in, line)) {
            line = trim(line);

            // Ignore blank lines and lines that start with #. A later # stays part of the value.
            if (line.empty() || line[0] == '#') { continue; }

            // Split at the first equals sign so the value may contain another equals sign.
            auto eq = line.find('=');

            if (eq == std::string::npos) { throw std::runtime_error("Expected key = value: " + line); }

            auto key = trim(line.substr(0, eq));

            if (seen[key]) { throw std::runtime_error("Repeated config key: " + key); }

            seen[key] = true;

            // Check the setting name now. Check its value after command-line overrides and `auto`
            // resolution are complete.
            assign(key, trim(line.substr(eq + 1)));
        }
    }
#pragma endregion

#pragma region /* Automatic-value resolution */
    // Apply command-line values last. A command-line value of `auto` asks the code below to calculate it.
    for (const auto& [k, v] : overrides) { assign(k, v); }

    // Start with all defaults for the selected RG-M target. Save any explicit geometry, A, Z, or GEMC
    // variation first so each one can replace its catalog default afterward.
    const std::string target_override = c.get("target");
    const std::string A_override = c.get("A");
    const std::string Z_override = c.get("Z");
    const std::string variation_override = c.get("gemc-target-variation");
    const auto& rgm_target = findRgmTarget(c.get("rgm-target"));
    c.values_["target"] = rgm_target.geometry;
    c.values_["A"] = std::to_string(rgm_target.A);
    c.values_["Z"] = std::to_string(rgm_target.Z);
    c.values_["gemc-target-variation"] = rgm_target.gemc_variation;

    // Apply these four overrides separately. A normal profile needs only `rgm-target`, while a special
    // study can replace one field without changing the others.
    if (target_override != "auto") { c.values_["target"] = target_override; }
    if (A_override != "auto") { c.values_["A"] = A_override; }
    if (Z_override != "auto") { c.values_["Z"] = Z_override; }
    if (variation_override != "auto") { c.values_["gemc-target-variation"] = variation_override; }

    if (uniform) {
        // Set the default angle limits for the particle and detector region. CD pions stop at 140
        // degrees, CD nucleons at 145, FD neutrons at 35, and charged FD hadrons at 45.
        const bool neutron = c.get("hadron") == "neutron";
        const bool pion = c.get("hadron") == "pip" || c.get("hadron") == "pim";
        const bool central = c.get("hadron-region") == "CD";
        if (c.get("hadron-theta-min") == "auto") { c.values_["hadron-theta-min"] = central ? "35" : "5"; }
        if (c.get("hadron-theta-max") == "auto") { c.values_["hadron-theta-max"] = central ? (pion ? "140" : "145") : (neutron ? "35" : "45"); }
        if (c.get("electron-p-max") == "auto") { c.values_["electron-p-max"] = c.get("beam-energy"); }
        if (c.get("hadron-p-min") == "auto") { c.values_["hadron-p-min"] = neutron ? "0" : c.get("hadron") == "proton" ? (central ? "0.2" : "0.3") : (central ? "0.1" : "0.2"); }

        // Use the established trigger-electron offsets for known beam energies. Other energies use zero.
        if (c.get("trigger-phi-offset") == "auto") {
            double e = c.number("beam-energy");
            c.values_["trigger-phi-offset"] = std::abs(e - 2.07052) < 1e-6 ? "16" : std::abs(e - 4.02962) < 1e-6 ? "7" : std::abs(e - 5.98636) < 1e-6 ? "5" : "0";
        }
    }

    // Use the established Q2-cut label for each known RG-M beam. Other energies use `none`.
    if (!uniform && c.get("q2-cut") == "auto") {
        const double e = c.number("beam-energy");
        c.values_["q2-cut"] = std::abs(e - 2.07052) < 1e-6 ? "Q2_0_02" : std::abs(e - 4.02962) < 1e-6 ? "Q2_0_25" : std::abs(e - 5.98636) < 1e-6 ? "Q2_0_40" : "none";
    }

    // Keep file prefixes short. The physical run-directory name below stores the longer source details.
    if (c.get("prefix") == "auto") {
        if (uniform) {
            std::ostringstream prefix;
            prefix << "Uniform_sample_" << uniformSampleLabel(c) << '_' << beamMeV(c.number("beam-energy")) << "MeV";
            c.values_["prefix"] = prefix.str();
        } else {
            c.values_["prefix"] = pathToken(c.get("rgm-target")) + '_' + pathToken(c.get("event-generator")) + '_' + std::to_string(beamMeV(c.number("beam-energy"))) + "MeV";
        }
    }

    if (uniform) {
        // Charged hadrons use the 50/50 p and 1/p mixture. Neutrons use uniform p. The trigger electron
        // in every eh event uses beam momentum.
        if (c.get("electron-momentum") == "auto") { c.values_["electron-momentum"] = c.get("channel") == "1e" ? "mixed" : "beam"; }
        if (c.get("hadron-momentum") == "auto" || c.get("hadron-momentum") == "sampled") {
            c.values_["hadron-momentum"] = c.get("channel") == "eh" && c.get("hadron") != "neutron" ? "mixed" : "uniform";
        }
    }
#pragma endregion

#pragma region /* Validation and path resolution */
    // Check settings before changing paths or adding the final run name. This creates no output.
    c.validate(uniform);

    // Leave URI-like inputs unchanged. Make local files and glob patterns absolute so later directory
    // changes do not change what they mean.
    if (!uniform && c.get("input").find("://") == std::string::npos) { c.values_["input"] = std::filesystem::absolute(c.get("input")).lexically_normal().string(); }

    if (uniform) {
        // Put uniform output in a channel-and-beam directory below the selected parent. A width of four
        // produces labels such as 2070, 4029, and 5986 without cutting off larger values.
        std::ostringstream directory;
        directory << "Uniform_sample_" << uniformSampleLabel(c) << '_' << std::setw(4) << std::setfill('0') << beamMeV(c.number("beam-energy")) << "MeV";
        c.values_["output"] = (std::filesystem::path(c.get("output")) / directory.str()).string();
    } else {
        // Put the physical source and simulation details in the directory name in the documented order.
        // Make each value path-safe here, but keep its original text in values_ for the manifest.
        std::ostringstream directory;
        directory << pathToken(c.get("gemc-target-variation")) << "__" << pathToken(c.get("event-generator")) << '-' << pathToken(c.get("event-generator-version")) << "__"
                  << pathToken(c.get("tune")) << "__" << pathToken(c.get("q2-cut")) << "__" << beamMeV(c.number("beam-energy")) << "MeV_GEMC-" << pathToken(c.get("gemc-version"));
        c.values_["output"] = (std::filesystem::path(c.get("output")) / directory.str()).string();
    }

    // Give later code one absolute run directory. The writer replaces it safely later.
    c.values_["output"] = std::filesystem::absolute(c.get("output")).lexically_normal().string();
#pragma endregion

    return c;
}
#pragma endregion

// RunConfig::get --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::get */
std::string RunConfig::get(const std::string& k) const {
    const auto value = values_.find(k);
    if (value == values_.end()) { throw std::out_of_range("Missing configuration key: " + k); }
    return value->second;
}
#pragma endregion

// RunConfig::number -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::number */
double RunConfig::number(const std::string& k) const {
    // `used` shows whether stod read the whole value or only a numeric prefix such as "5" in "5 GeV".
    std::size_t used = 0;
    double value = std::stod(get(k), &used);

    // Some libraries let stod read NaN or infinity. Reject them and any text left after the number.
    if (used != get(k).size() || !std::isfinite(value)) { throw std::runtime_error("Invalid number: " + k); }

    return value;
}
#pragma endregion

// RunConfig::integer ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::integer */
std::uint64_t RunConfig::integer(const std::string& k) const {
    // Require digits before conversion so stoull cannot accept a sign or surrounding whitespace.
    const auto s = get(k);

    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) { throw std::runtime_error("Expected unsigned integer: " + k); }

    // stoull performs the final range check after the digits-only check.
    return std::stoull(s);
}
#pragma endregion

// RunConfig::validate ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::validate */
void RunConfig::validate(bool uniform) const {
#pragma region /* Shared run and output contract */
    // At this point, output is still the parent selected by the user. parse() adds the run name after
    // validation. Both LUND sources require an event count.
    if (get("output").empty()) { throw std::runtime_error("--output is required; use a new run directory"); }
    if (get("events").empty()) { throw std::runtime_error("--events is required; provide the total number of events to write"); }

    // Beam energy must be a finite, positive value in GeV. number() checks its text and finiteness.
    if (number("beam-energy") <= 0) { throw std::runtime_error("beam-energy must be positive"); }

    // Event counts must be positive. Seeds may be zero because TRandom3(0) asks ROOT to choose a new,
    // nonrepeatable seed. A run that uses zero cannot be reproduced from the recorded value alone.
    for (auto k : {"events", "events-per-file"}) {
        auto n = integer(k);
        if (!n || n > std::numeric_limits<unsigned int>::max()) { throw std::runtime_error(std::string(k) + " must be in [1, 4294967295]"); }
    }
    for (auto k : {"seed", "vertex-seed"}) {
        if (integer(k) > std::numeric_limits<unsigned int>::max()) { throw std::runtime_error(std::string(k) + " must be in [0, 4294967295]"); }
    }

    // A and Z are LUND header values and do not choose the vertex geometry. integer() already requires
    // Z to be nonnegative; these checks keep A and Z physically ordered.
    if (integer("A") < 1 || integer("A") > 300 || integer("Z") > integer("A")) { throw std::runtime_error("Require 1 <= A <= 300 and 0 <= Z <= A"); }

    // The prefix is used in filenames, so allow only a nonempty portable name.
    if (get("prefix").empty() || get("prefix").find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos) {
        throw std::runtime_error("prefix must contain only letters, numbers, _, . or -");
    }

#pragma endregion

#pragma region /* Vertex contract */
    // Every event samples the selected target geometry. There is no fixed-vertex mode.
    TargetGeometry::validate(get("target"));
#pragma endregion

#pragma region /* Source-specific contract */
    if (!uniform) {
        // Physical mode currently supports only GENIE GST ROOT input. Require a path or glob now; the
        // adapter opens the files and checks the tree when conversion starts.
        if (get("input").empty()) { throw std::runtime_error("--input GST ROOT file or glob is required"); }
        if (get("event-generator") != "genie-gst") { throw std::runtime_error("Only --event-generator genie-gst is currently implemented"); }

        // These values are used in the physical directory name and manifest. `unknown` and `none` are
        // allowed, but an empty value is not.
        for (auto k : {"event-generator-version", "tune", "q2-cut", "gemc-version", "gemc-target-variation"}) {
            if (get(k).empty()) { throw std::runtime_error(std::string(k) + " must not be empty"); }
        }

        // Physical configurations have no uniform-only settings, so their checks end here.
        return;
    }

    // Uniform mode accepts one electron, an electron-hadron pair, or the electron tester. These are
    // acceptance samples, not physical interaction models.
    if (get("channel") != "1e" && get("channel") != "eh" && get("channel") != "electron-tester") { throw std::runtime_error("channel must be 1e, eh or electron-tester"); }
    if (get("hadron") != "proton" && get("hadron") != "neutron" && get("hadron") != "pip" && get("hadron") != "pim") {
        throw std::runtime_error("hadron must be proton, neutron, pip or pim");
    }
    if (get("hadron-region") != "FD" && get("hadron-region") != "CD") { throw std::runtime_error("hadron-region must be FD or CD"); }

    // Require increasing polar-angle limits between 0 and 180 degrees. A profile may narrow the defaults.
    for (auto stem : {"electron", "hadron"}) {
        double lo = number(std::string(stem) + "-theta-min"), hi = number(std::string(stem) + "-theta-max");
        if (!(0 <= lo && lo < hi && hi <= 180)) { throw std::runtime_error("Require 0 <= theta-min < theta-max <= 180"); }
    }

    // parse() resolves `auto` and `sampled` before this check. Mixed sampling needs a positive minimum
    // because 1/p is undefined at zero. Fixed momentum is allowed only for neutrons.
    if (get("hadron-momentum") != "fixed" && get("hadron-momentum") != "uniform" && get("hadron-momentum") != "mixed") {
        throw std::runtime_error("hadron-momentum must be fixed, sampled, uniform or mixed");
    }
    if (get("hadron-momentum") == "mixed" && (get("channel") != "eh" || get("hadron") == "neutron" || number("hadron-p-min") <= 0)) {
        throw std::runtime_error("mixed requires an eh charged hadron and strictly positive hadron-p-min");
    }
    if (get("hadron-momentum") == "fixed" && (get("channel") != "eh" || get("hadron") != "neutron")) {
        throw std::runtime_error("fixed hadron momentum is available only for eh neutron samples");
    }

    if (get("electron-momentum") != "uniform" && get("electron-momentum") != "mixed" && get("electron-momentum") != "beam") {
        throw std::runtime_error("electron-momentum must be auto, uniform, mixed or beam");
    }
    if (get("electron-momentum") == "mixed" && (get("channel") != "1e" || number("electron-p-min") <= 0)) {
        throw std::runtime_error("mixed electron momentum requires 1e and strictly positive electron-p-min");
    }

    // Fixed momentum must be positive. Uniform sampling may start at zero, but every sampled range must
    // have a positive width. The mixed-mode checks above require a positive minimum.
    if (number("hadron-p") <= 0 || number("hadron-p-min") < 0 || number("beam-energy") <= number("hadron-p-min")) { throw std::runtime_error("Invalid hadron momentum bounds"); }
    if (number("electron-p-min") < 0 || number("electron-p-max") <= number("electron-p-min")) { throw std::runtime_error("Invalid electron momentum bounds"); }

    // Trigger theta must be a valid polar angle, and the sector offset must stay within ±180 degrees.
    if (number("trigger-theta") < 0 || number("trigger-theta") > 180 || std::abs(number("trigger-phi-offset")) > 180) { throw std::runtime_error("Invalid trigger angle"); }
#pragma endregion
}
#pragma endregion

// jsonString ------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* jsonString */
std::string jsonString(const std::string& s) {
    // Build a new string without changing the original setting.
    std::ostringstream out;
    out << '"';

    // Read unsigned bytes so control-character checks work the same on every platform.
    for (unsigned char ch : s) {
        // Escape quotes and backslashes because JSON uses them as syntax.
        if (ch == '"' || ch == '\\') {
            out << '\\' << ch;
        } else if (ch < 0x20) {
            // JSON does not allow raw control bytes. Write a four-digit Unicode escape, then switch the
            // stream back to decimal numbers.
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
        } else {
            // Ordinary text and UTF-8 bytes need no extra escaping here.
            out << ch;
        }
    }

    // Add the closing quote so the result is ready to use as one JSON key or value.
    out << '"';

    return out.str();
}
#pragma endregion

// help ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* help */
std::string help(bool uniform) {
    // Quote the physical input glob so the shell passes the pattern to the converter unchanged.
    std::string result = uniform ? "uniform-lund-creator --channel 1e|eh|electron-tester [--hadron proton|neutron|pip|pim --hadron-region FD|CD] --output PARENT_DIRECTORY\n"
                                 : "event-generator-to-lund-converter --event-generator genie-gst --input 'gst*.root' --output PARENT_DIRECTORY\n";

    // List the settings shared by both LUND sources.
    result +=
        "Settings: --config FILE, --beam-energy GeV, --rgm-target ID, --target GEOMETRY, --A N, --Z N,\n"
        "--events N, --events-per-file N, --seed N, --vertex-seed N, --prefix NAME,\n"
        "Every event samples the selected target geometry.\n"
        "Files use key = value; CLI values override file settings. Seed 0 requests ROOT automatic, nonrepeatable seeding.\n"
        "Existing output is replaced after a warning.\n";
    if (uniform) { result += "Uniform event IDs start at zero and continue across split files.\n"; }

    if (uniform) {
        // List the uniform-only acceptance and sampling settings. parse() resolves automatic values
        // before validation.
        result +=
            "Uniform: --hadron proton|neutron|pip|pim, --hadron-region FD|CD, --electron-theta-min/max DEG, --electron-p-min/max GeV/c,\n"
            "--hadron-theta-min/max DEG, --electron-momentum auto|uniform|mixed|beam,\n"
            "--hadron-momentum auto|fixed|sampled|uniform|mixed,\n"
            "--hadron-p GeV/c, --hadron-p-min GeV/c, --trigger-theta DEG, --trigger-phi-offset DEG.\n"
            "Hadron theta and phi are always sampled uniformly inside their configured ranges.\n"
            "Sampled hadron momentum extends to the beam energy. Uniform monitoring is always written and rendered.\n";
    } else {
        // List the physical-only GENIE GST and source-description settings.
        result +=
            "Physical: --event-generator genie-gst (default), --event-generator-version VERSION, --tune NAME,\n"
            "--q2-cut NAME, --gemc-version VERSION, --gemc-target-variation NAME.\n"
            "For physical input, --events-per-file also sets the minimum inclusive input block required before a follow-up file starts, aligned with JOB_NEVENTS.\n";
    }

    // Read supported names from the target catalog instead of copying the list here.
    result += "RG-M targets: " + rgmTargetNames() + "\n";

    return result;
}
#pragma endregion

}  // namespace samples
