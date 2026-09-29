//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.cpp
 * @brief Combines defaults, a configuration file, and command-line values into checked run settings.
 *
 * Purpose:
 *   Prepare every setting before event work begins. The uniform LUND creator creates new random test events.
 *   The physical LUND converter reads existing event-generator events and converts them to LUND. Both paths
 *   receive one checked configuration, and the run manifest records its final values.
 *
 * Execution flow:
 *   Begin with built-in defaults. Replace them with values from one optional configuration file. Replace
 *   those with command-line values. Calculate settings marked `auto`, check the complete result, and make
 *   local input and output paths absolute.
 *
 * CLI options (shared):
 *   --config FILE                       Read `key = value` settings (default: no configuration file).
 *   --beam-energy GeV                   Set the beam energy (default: 5.98636 GeV).
 *   --target ID                         Select the target material (default: Ar40).
 *   --gemc-target-variation NAME        Select a compatible GEMC target setup (default: auto).
 *   --A N                               Set the LUND target A value (default: selected target's A).
 *   --Z N                               Set the LUND target Z value (default: selected target's Z).
 *   --output DIRECTORY                  Select the parent output directory (required).
 *   --events N                          Set the number of events to write (required).
 *   --events-per-file N                 Split output after N events (default: uniform 25000; physical 10000).
 *   --seed N                            Set the uniform particle-motion seed (default: 67890).
 *   --vertex-seed N                     Set the vertex-position seed (default: 12345).
 *   --prefix NAME                       Set the LUND filename prefix (default: auto from run settings).
 *
 * CLI options (uniform LUND creator):
 *   --channel 1e|eh|electron-tester     Select the created final state (default: 1e).
 *   --hadron proton|neutron|pip|pim     Select the hadron for `eh` (default: proton).
 *   --hadron-region FD|CD               Select the hadron detector region (default: FD).
 *   --electron-theta-min DEG            Set the electron theta minimum (default: 5 degrees).
 *   --electron-theta-max DEG            Set the electron theta maximum (default: 40 degrees).
 *   --electron-p-min GeV/c              Set the electron momentum minimum (default: 0.7 GeV/c).
 *   --electron-p-max GeV/c              Set the electron momentum maximum (default: beam momentum).
 *   --electron-momentum MODE            Select uniform, mixed, or beam momentum (default: auto by channel).
 *   --hadron-theta-min DEG              Set the hadron theta minimum (default: auto by particle and region).
 *   --hadron-theta-max DEG              Set the hadron theta maximum (default: auto by particle and region).
 *   --hadron-p-min GeV/c                Set the hadron momentum minimum (default: auto by particle and region).
 *   --hadron-p GeV/c                    Set the fixed neutron momentum (default: 1 GeV/c).
 *   --hadron-momentum MODE              Select fixed, uniform, or mixed momentum (default: auto by channel and hadron).
 *   --trigger-theta DEG                 Set the trigger-electron theta (default: 25 degrees).
 *   --trigger-phi-offset DEG            Set its opposite-sector offset (default: auto by beam energy).
 *
 * CLI options (physical LUND converter):
 *   --input GST_GLOB                    Select GENIE GST ROOT input files (required).
 *   --event-generator genie-gst         Select the physical-input adapter (default: genie-gst).
 *   --event-generator-version VERSION   Record the event-generator version (default: unknown).
 *   --tune NAME                         Record the GENIE tune (default: auto from input_options.txt, then unknown).
 *   --q2-cut NAME                       Record the input selection label (default: auto by beam energy; no cut applied).
 *   --output-layout nested|metadata     Select the physical run-directory layout (default: nested).
 *
 * Configuration-file syntax:
 *   Use the same setting names without `--` and write each one as `key = value`. `config` is a command-line
 *   option, not a setting inside the file. The application handles `--help` before createFromCommandLine().
 *
 * Setting priority:
 *   A command-line value replaces the same configuration-file value. A configuration-file value replaces
 *   the built-in default. The code calculates `auto` only after all user-supplied values are known.
 *
 * Scope:
 *   This file only prepares settings. It does not create events, read event data, choose random values,
 *   replace output directories, write LUND records, create monitoring plots, or submit simulation jobs.
 */

#include "core/config/RunConfig.h"

#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "core/config/TargetCatalog.h"
#include "core/geometry/TargetGeometry.h"

namespace samples {

// Private configuration helpers -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private configuration helpers */
/**
 * @namespace samples::<anonymous>
 * @brief Contains small configuration functions that only this source file may call.
 *
 * These functions clean text, try to find a GENIE tune, and build names for output files and directories.
 * They are hidden from the rest of the project.
 */
namespace {

// Removing outside spaces -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Removing outside spaces */
/**
 * @brief Remove spaces and line-ending characters from both ends of text.
 * @param s Copy of the text to clean.
 * @return Text without spaces, tabs, carriage returns, or newlines at either end. Empty input or input
 *         containing only those characters returns an empty string.
 * @note Characters inside the text, including spaces, stay unchanged.
 */
std::string trim(std::string s) {
    // If no kept character exists, the input was empty or contained only removable characters.
    auto first = s.find_first_not_of(" \t\r\n");

    // Copy from the first kept character through the last one.
    return (first == std::string::npos) ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
#pragma endregion

// Finding the GENIE tune ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Finding the GENIE tune */
/**
 * @brief Try to read the GENIE tune recorded beside the input files.
 *
 * Execution flow:
 *   Start from the input file's directory. For an input pattern such as `gst*.root`, start from the fixed
 *   directory before the wildcard. Move upward until finding a directory named
 *   `master-routine_validation_01-eScattering`. Open `input_options.txt` in its parent directory and
 *   return the value written after the exact `TUNE` key.
 *
 * @param input Local GST ROOT filename or wildcard pattern.
 * @return The recorded tune. Returns `unknown` for a remote input, an unexpected directory layout, an
 *         unreadable metadata file, or a missing or empty `TUNE` value.
 * @note Failure to discover the tune does not stop conversion and does not change any file. Supplying
 *       `--tune` skips this search.
 */
std::string discoverGenieTune(const std::string& input) {
    // There is no local directory to search when no input was supplied. `://` identifies a remote address,
    // such as `root://server/path/file.root`, whose surrounding files cannot be opened as local files.
    if (input.empty() || (input.find("://") != std::string::npos)) { return "unknown"; }

    // A wildcard is a character such as `*` in `gst*.root`. It represents several possible filenames rather
    // than one real file. Find the first wildcard so we can keep only the fixed directory before it.
    const auto wildcard = input.find_first_of("*?[");
    std::filesystem::path directory;

    if (wildcard == std::string::npos) {
        // A normal filename has no wildcard, so begin in the directory that contains that file.
        directory = std::filesystem::path(input).parent_path();
    } else {
        // For `/data/run/gst*.root`, the last slash before `*` separates the searchable directory
        // `/data/run` from the filename pattern. A pattern without such a directory gives us nowhere to start.
        const auto separator = input.find_last_of("/\\", wildcard);
        if (separator == std::string::npos) { return "unknown"; }

        directory = std::filesystem::path(input.substr(0, separator));
    }

    // Convert the starting directory to a complete, cleaned path. This lets the upward search compare one
    // unambiguous directory at a time even when the user supplied `.` or `..` in the input path.
    directory = std::filesystem::absolute(directory).lexically_normal();

    // Check the starting directory, then its parent, then the parent's parent, until the expected production
    // directory is found or the top of the filesystem is reached.
    while (!directory.empty()) {
        if (directory.filename() == "master-routine_validation_01-eScattering") {
            // GENIE production records its settings in `input_options.txt` beside this production directory.
            std::ifstream metadata(directory.parent_path() / "input_options.txt");
            std::string line;

            // Read the file one line at a time. Split each line into words and use only a line whose first
            // word is exactly `TUNE`; the following word is the tune name recorded in the run settings.
            while (std::getline(metadata, line)) {
                std::istringstream fields(line);
                std::string key;
                std::string value;

                if ((fields >> key) && (key == "TUNE")) { return (fields >> value) && !value.empty() ? value : "unknown"; }
            }

            return "unknown";
        }

        // At the filesystem root, asking for the parent returns the same directory. Stop there so this loop
        // cannot repeat forever; otherwise continue the search one directory higher.
        const auto parent = directory.parent_path();
        if (parent == directory) { break; }

        directory = parent;
    }

    return "unknown";
}
#pragma endregion

// Preparing one directory-name part -------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Preparing one directory-name part */
/**
 * @brief Turn one setting into a single safe part of a directory name.
 *
 * Purpose:
 *   Output names include settings such as the target and tune. A slash or another special character in a
 *   setting must not create an extra directory or change the path. This function replaces such characters
 *   while keeping the name readable.
 *
 * Conversion:
 *   Keep letters, digits, `.`, `_`, and `-`. Replace every other character with `-`. Reject empty text
 *   and the special directory names `.` and `..`.
 *
 * @param value Final setting text copied from the configuration.
 * @return Text that can be used as one directory-name part.
 * @throws std::runtime_error If the resulting component is empty, `.` or `..`.
 * @note Two different inputs may become the same directory-name text. The manifest keeps the original
 *       value.
 */
std::string pathToken(std::string value) {
    // std::isalnum requires a nonnegative byte value. Keep only the three allowed punctuation marks.
    for (char& ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || (ch == '.') || (ch == '_') || (ch == '-'))) { ch = '-'; }
    }

    // `.` means the current directory and `..` means its parent, so neither is allowed as a generated
    // name.
    if (value.empty() || (value == ".") || (value == "..")) { throw std::runtime_error("Invalid empty output-name component"); }

    return value;
}
#pragma endregion

// Building the beam-energy label ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Building the beam-energy label */
/**
 * @brief Convert a beam energy in GeV to the whole-number MeV label used in output names.
 *
 * Purpose:
 *   Keep the established labels `2070`, `4029`, and `5986` for the three known beam energies. Build a
 *   normally rounded label for any other energy.
 *
 * Conversion:
 *   Use a fixed label when the energy differs from a known value by less than `0.000001` GeV. Otherwise,
 *   multiply by 1000 and round to the nearest whole number.
 *
 * @param energy Finite beam energy in GeV.
 * @return Beam-energy label in MeV for filenames and directory names.
 * @note 2.07052 GeV intentionally produces `2070`, not the normally rounded value `2071`.
 */
long long beamMeV(double energy) {
    if (std::abs(energy - 2.07052) < 1e-6) { return 2070; }
    if (std::abs(energy - 4.02962) < 1e-6) { return 4029; }
    if (std::abs(energy - 5.98636) < 1e-6) { return 5986; }

    return std::llround(energy * 1000.0);
}
#pragma endregion

// Building the uniform-sample label -------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Building the uniform-sample label */
/**
 * @brief Build the short particle-and-detector label used in uniform output names.
 * @param config Final configuration containing the channel, hadron, and detector region.
 * @return `1e` for one electron, `electron-tester` for the electron test, or a label such as `epFD` for
 *         an electron, an FD proton.
 */
std::string uniformSampleLabel(const RunConfig& config) {
    if (config.getText("channel") == "1e") { return "1e"; }
    if (config.getText("channel") == "electron-tester") { return "electron-tester"; }

    const auto& hadron = config.getText("hadron");

    const std::string token = (hadron == "proton") ? "p" : (hadron == "neutron") ? "n" : hadron;

    // Put `e` before the hadron token and the detector region after it. Proton and neutron use the shortened
    // tokens above, producing labels such as `epFD` and `enCD`. The pion names remain `pip` and `pim`, so
    // they produce labels such as `epipFD` and `epimCD`.
    return "e" + token + config.getText("hadron-region");
}
#pragma endregion

}  // namespace
#pragma endregion

// Reading and preparing settings ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Reading and preparing settings */
RunConfig RunConfig::createFromCommandLine(int argc, char** argv, LundSource source) {
    const bool uniform = (source == LundSource::Uniform);

#pragma region /* Built-in defaults */
    // Keep settings as text so the manifest records their exact final values. These defaults select the
    // usual RG-M beam and argon target. Particle motion and vertex positions use separate random seeds.
    // Particle masses come from targets.h and are not configuration settings.
    RunConfig c;
    c.values_ = {{"beam-energy", "5.98636"},
                 {"target", "Ar40"},
                 {"A", "auto"},
                 {"Z", "auto"},
                 {"gemc-target-variation", "auto"},
                 {"output", ""},
                 {"events", ""},
                 {"events-per-file", uniform ? "25000" : "10000"},
                 {"seed", "67890"},
                 {"vertex-seed", "12345"},
                 {"prefix", "auto"}};

    // Add only settings used by the selected kind of event work. A setting for the other kind is reported
    // as unknown instead of being silently ignored.
    if (uniform) {
        // The uniform LUND creator creates random test events. Values marked `auto` are calculated after
        // the configuration file and command line have been read. A fixed momentum of 1 GeV/c is
        // available only for neutrons.
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
        // The physical LUND converter reads particles from existing event-generator files and records
        // their origin. It does not run GENIE. The currently supported input is a GENIE GST ROOT tree.
        c.values_.insert({{"input", ""}, {"event-generator", "genie-gst"}, {"event-generator-version", "unknown"}, {"tune", "auto"}, {"q2-cut", "auto"}, {"output-layout", "nested"}});
    }

    // Use one check for names read from both the configuration file and the command line.
    auto assign = [&](const std::string& k, const std::string& v) {
        if (!c.values_.count(k)) { throw std::runtime_error("Unknown setting: " + k); }

        c.values_[k] = v;
    };
#pragma endregion

#pragma region /* Configuration-file and command-line input */
    // Save command-line values until after the configuration file is read, because command-line values
    // have higher priority. The map also makes repeated command-line settings easy to detect.
    std::map<std::string, std::string> overrides;
    std::string config;

    // Every command-line setting is a `--key value` pair. A negative number after a key is treated as its
    // value and will later fail only if that setting does not allow negative numbers.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg.rfind("--", 0) != 0) || (i + 1 == argc)) { throw std::runtime_error("Expected --key value: " + arg); }

        auto key = arg.substr(2);
        std::string value = argv[++i];

        // `config` names the configuration file to read. It is not part of the final run settings.
        if (key == "config") {
            if (!config.empty()) { throw std::runtime_error("Use only one --config file"); }

            config = value;
        } else {
            if (!overrides.emplace(key, value).second) { throw std::runtime_error("Repeated option: " + key); }
        }
    }

    // Read at most one configuration file and report repeated names. A relative file path starts from the
    // process's current directory. workflow.py starts project commands from the repository root.
    if (!config.empty()) {
        std::ifstream in(config);

        if (!in) { throw std::runtime_error("Cannot open config: " + config); }

        std::string line;
        std::map<std::string, bool> seen;

        while (std::getline(in, line)) {
            line = trim(line);

            // Ignore empty lines and comment lines. A # after the first character remains part of the
            // setting value.
            if (line.empty() || (line[0] == '#')) { continue; }

            // Split at the first equals sign so another equals sign may remain inside the value.
            auto eq = line.find('=');

            if (eq == std::string::npos) { throw std::runtime_error("Expected key = value: " + line); }

            auto key = trim(line.substr(0, eq));

            if (seen[key]) { throw std::runtime_error("Repeated config key: " + key); }

            seen[key] = true;

            // Check the setting name now. Check the value later, after command-line replacement and
            // automatic-value calculation are complete.
            assign(key, trim(line.substr(eq + 1)));
        }
    }
#pragma endregion

#pragma region /* Automatic-value resolution */
    // Apply command-line values last so they replace configuration-file values. A command-line value of
    // `auto` asks the following code to calculate that setting.
    for (const auto& [k, v] : overrides) { assign(k, v); }

    // Find the selected target material. Its A value is the total number of protons and neutrons, and its
    // Z value is the number of protons. The target and beam energy select the usual GEMC target setup. That
    // setup also supplies the targets.h geometry used to choose vertex positions. The user may name a
    // different compatible GEMC setup for an unusual run.
    const std::string A_override = c.getText("A");
    const std::string Z_override = c.getText("Z");
    const std::string variation_override = c.getText("gemc-target-variation");
    const auto& target = findTarget(c.getText("target"));
    const auto& variation = resolveTargetVariation(target, c.getDouble("beam-energy"), variation_override);
    c.values_["A"] = std::to_string(target.A);
    c.values_["Z"] = std::to_string(target.Z);
    c.values_["gemc-target-variation"] = variation.identifier;
    c.values_["target-geometry"] = variation.geometry;

    // A and Z describe the nucleus in the LUND header, not its shape or position. Keep explicit A and Z
    // values even when they differ from the selected target's defaults.
    if (A_override != "auto") { c.values_["A"] = A_override; }
    if (Z_override != "auto") { c.values_["Z"] = Z_override; }

    if (uniform) {
        // Fill the default hadron-angle limits for the selected particle and detector region. CD means the
        // central detector and FD means the forward detector. CD pions stop at 140 degrees, CD protons and
        // neutrons at 145, FD neutrons at 35, and charged FD hadrons at 45.
        const bool neutron = (c.getText("hadron") == "neutron");
        const bool pion = ((c.getText("hadron") == "pip") || (c.getText("hadron") == "pim"));
        const bool central = (c.getText("hadron-region") == "CD");
        if (c.getText("hadron-theta-min") == "auto") { c.values_["hadron-theta-min"] = central ? "35" : "5"; }
        if (c.getText("hadron-theta-max") == "auto") { c.values_["hadron-theta-max"] = central ? (pion ? "140" : "145") : (neutron ? "35" : "45"); }
        if (c.getText("electron-p-max") == "auto") { c.values_["electron-p-max"] = c.getText("beam-energy"); }
        if (c.getText("hadron-p-min") == "auto") { c.values_["hadron-p-min"] = neutron ? "0" : (c.getText("hadron") == "proton") ? (central ? "0.2" : "0.3") : (central ? "0.1" : "0.2"); }

        // Choose the usual angle between the trigger electron and the opposite detector sector for each
        // known beam energy. Other beam energies use no offset.
        if (c.getText("trigger-phi-offset") == "auto") {
            double e = c.getDouble("beam-energy");
            c.values_["trigger-phi-offset"] = (std::abs(e - 2.07052) < 1e-6) ? "16" : (std::abs(e - 4.02962) < 1e-6) ? "7" : (std::abs(e - 5.98636) < 1e-6) ? "5" : "0";
        }
    }

    // Choose the usual Q²-cut label for each known RG-M beam energy. Q² describes the squared momentum
    // transferred in the interaction. Other energies use `none`. Convert accepted underscore spellings
    // to one standard spelling so equivalent values produce the same output name.
    if (!uniform) {
        if (c.getText("q2-cut") == "auto") {
            const double e = c.getDouble("beam-energy");
            c.values_["q2-cut"] = (std::abs(e - 2.07052) < 1e-6) ? "Q2-0.02" : (std::abs(e - 4.02962) < 1e-6) ? "Q2-0.25" : (std::abs(e - 5.98636) < 1e-6) ? "Q2-0.40" : "none";
        } else if ((c.getText("q2-cut") == "Q2_0_02") || (c.getText("q2-cut") == "Q2_0.02")) {
            c.values_["q2-cut"] = "Q2-0.02";
        } else if ((c.getText("q2-cut") == "Q2_0_25") || (c.getText("q2-cut") == "Q2_0.25")) {
            c.values_["q2-cut"] = "Q2-0.25";
        } else if ((c.getText("q2-cut") == "Q2_0_40") || (c.getText("q2-cut") == "Q2_0.40")) {
            c.values_["q2-cut"] = "Q2-0.40";
        }
    }

    // A GENIE tune names the physics settings used to produce the input events. Standard productions store
    // it in input_options.txt near the input files. If it cannot be read, record `unknown` and continue.
    if (!uniform && (c.getText("tune") == "auto")) { c.values_["tune"] = discoverGenieTune(c.getText("input")); }

    // Build a filename prefix that still describes the sample if a LUND file is copied out of its run
    // directory. Double underscores separate different facts. A hyphen joins an event-generator name to
    // its version. Physical prefixes always include the tune and Q²-cut label. The manifest records the
    // generator version even when it is `unknown`.
    if (c.getText("prefix") == "auto") {
        if (uniform) {
            std::ostringstream prefix;
            prefix << "Uniform__" << uniformSampleLabel(c) << "__" << beamMeV(c.getDouble("beam-energy")) << "MeV";
            c.values_["prefix"] = prefix.str();
        } else {
            std::ostringstream prefix;
            prefix << pathToken(c.getText("target")) << "__" << pathToken(c.getText("event-generator"));
            if (c.getText("event-generator-version") != "unknown") { prefix << "-" << pathToken(c.getText("event-generator-version")); }
            prefix << "__" << pathToken(c.getText("tune")) << "__" << pathToken(c.getText("q2-cut")) << "__" << beamMeV(c.getDouble("beam-energy")) << "MeV";
            c.values_["prefix"] = prefix.str();
        }
    }

    if (uniform) {
        // For charged hadrons, `mixed` chooses half the momenta uniformly in p and half uniformly in 1/p.
        // Neutrons use uniform p. In an electron-hadron event, the trigger electron uses beam momentum.
        if (c.getText("electron-momentum") == "auto") { c.values_["electron-momentum"] = (c.getText("channel") == "1e") ? "mixed" : "beam"; }
        if ((c.getText("hadron-momentum") == "auto") || (c.getText("hadron-momentum") == "sampled")) {
            c.values_["hadron-momentum"] = ((c.getText("channel") == "eh") && (c.getText("hadron") != "neutron")) ? "mixed" : "uniform";
        }
    }
#pragma endregion

#pragma region /* Final checks and paths */
    // Check the settings before changing paths or adding the final run-directory name. Validation does not
    // create or change any file or directory.
    c.validateForSource(source);

    // Leave remote addresses containing :// unchanged. Make local filenames and wildcard patterns absolute
    // so they still refer to the same input if later code changes its current directory.
    if (!uniform && (c.getText("input").find("://") == std::string::npos)) { c.values_["input"] = std::filesystem::absolute(c.getText("input")).lexically_normal().string(); }

    if (uniform) {
        // Add a directory name containing the uniform channel and beam energy below the parent directory
        // supplied by the user. Use the same facts as the automatic filename prefix.
        std::ostringstream directory;
        directory << "Uniform__" << uniformSampleLabel(c) << "__" << std::setw(4) << std::setfill('0') << beamMeV(c.getDouble("beam-energy")) << "MeV";
        c.values_["output"] = (std::filesystem::path(c.getText("output")) / directory.str()).string();
    } else if (c.getText("output-layout") == "nested") {
        // The nested layout uses three directory levels: target, event generator and tune, then Q² cut and
        // beam energy. Double underscores separate facts within one directory name.
        const auto generator_and_tune = pathToken(c.getText("event-generator")) + "__" + pathToken(c.getText("tune"));
        const auto selection_and_beam = pathToken(c.getText("q2-cut")) + "__" + std::to_string(beamMeV(c.getDouble("beam-energy"))) + "MeV";
        c.values_["output"] = (std::filesystem::path(c.getText("output")) / pathToken(c.getText("target")) / generator_and_tune / selection_and_beam).string();
    } else {
        // The metadata layout puts every fact in one directory name. A hyphen joins the event generator
        // and its version. Double underscores separate that pair from the other facts.
        std::ostringstream directory;
        directory << pathToken(c.getText("gemc-target-variation")) << "__" << pathToken(c.getText("event-generator")) << "-" << pathToken(c.getText("event-generator-version")) << "__"
                  << pathToken(c.getText("tune")) << "__" << pathToken(c.getText("q2-cut")) << "__" << beamMeV(c.getDouble("beam-energy")) << "MeV";
        c.values_["output"] = (std::filesystem::path(c.getText("output")) / directory.str()).string();
    }

    // Store one absolute run-directory path. The writer later checks that exact path before replacing it.
    c.values_["output"] = std::filesystem::absolute(c.getText("output")).lexically_normal().string();
#pragma endregion

    return c;
}
#pragma endregion

// Reading one text setting ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Reading one text setting */
std::string RunConfig::getText(const std::string& k) const {
    const auto value = values_.find(k);

    if (value == values_.end()) { throw std::out_of_range("Missing configuration key: " + k); }

    return value->second;
}
#pragma endregion

// Reading one decimal number --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Reading one decimal number */
double RunConfig::getDouble(const std::string& k) const {
    // stod reports how many characters it converted through `used`. This reveals extra text: for example,
    // it can read the number 5 from `5 GeV`, but used will show that ` GeV` remains.
    std::size_t used = 0;
    double value = std::stod(getText(k), &used);

    // Reject extra text, infinity, and NaN, which means a value that is not a valid number.
    if ((used != getText(k).size()) || !std::isfinite(value)) { throw std::runtime_error("Invalid number: " + k); }

    return value;
}
#pragma endregion

// Reading one nonnegative whole number ----------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Reading one nonnegative whole number */
std::uint64_t RunConfig::getNonnegativeInteger(const std::string& k) const {
    // Require digits only. This rejects a minus or plus sign, a decimal point, and surrounding spaces
    // before conversion.
    const auto s = getText(k);

    if (s.empty() || (s.find_first_not_of("0123456789") != std::string::npos)) { throw std::runtime_error("Expected unsigned integer: " + k); }

    // stoull converts the digits and reports an error if the value is too large for the return type.
    return std::stoull(s);
}
#pragma endregion

// Checking all final settings -------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Checking all final settings */
void RunConfig::validateForSource(LundSource source) const {
    const bool uniform = (source == LundSource::Uniform);

#pragma region /* Settings used by both event sources */
    // At this point, output is still the parent directory supplied by the user. createFromCommandLine() adds the final run
    // name after these checks. Both kinds of event work require a requested event count.
    if (getText("output").empty()) { throw std::runtime_error("--output is required; use a new run directory"); }
    if (getText("events").empty()) { throw std::runtime_error("--events is required; provide the total number of events to write"); }

    // Beam energy is measured in GeV and must be greater than zero. getDouble() has already rejected extra
    // text, infinity, and a value that is not a number.
    if (getDouble("beam-energy") <= 0) { throw std::runtime_error("beam-energy must be positive"); }

    // Event counts must be between 1 and the largest value accepted by the event loops. A random seed may
    // be zero. ROOT treats TRandom3(0) as a request to choose a new seed, so a zero-seeded run cannot be
    // repeated later from the recorded zero alone.
    for (auto k : {"events", "events-per-file"}) {
        auto n = getNonnegativeInteger(k);
        if (!n || (n > std::numeric_limits<unsigned int>::max())) { throw std::runtime_error(std::string(k) + " must be in [1, 4294967295]"); }
    }
    for (auto k : {"seed", "vertex-seed"}) {
        if (getNonnegativeInteger(k) > std::numeric_limits<unsigned int>::max()) { throw std::runtime_error(std::string(k) + " must be in [0, 4294967295]"); }
    }

    // In the LUND header, A is the total number of protons and neutrons and Z is the number of protons.
    // They do not choose the target shape or vertex positions. Require a possible ordering: at least one
    // particle in the nucleus and no more protons than the total.
    if ((getNonnegativeInteger("A") < 1) || (getNonnegativeInteger("A") > 300) || (getNonnegativeInteger("Z") > getNonnegativeInteger("A"))) {
        throw std::runtime_error("Require 1 <= A <= 300 and 0 <= Z <= A");
    }

    // The prefix becomes part of each output filename. Limit it to characters that work reliably in common
    // filesystems and command lines.
    if (getText("prefix").empty() || (getText("prefix").find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos)) {
        throw std::runtime_error("prefix must contain only letters, numbers, _, . or -");
    }

#pragma endregion

#pragma region /* Vertex-position setting */
    // Every event receives one vertex position from the selected target geometry. targets.h samples Vx
    // and Vy from Gaussian beam-spot distributions. It samples Vz uniformly across liquid and Ar cells;
    // for other geometries it places Vz at one listed target-component center. This workflow does not
    // support an option that replaces those geometry rules with fixed user-supplied coordinates.
    TargetGeometry::validateGeometryName(getText("target-geometry"));
#pragma endregion

#pragma region /* Settings for the selected event source */
    if (!uniform) {
        // The physical LUND converter currently reads only GENIE GST ROOT input. Require a filename or
        // wildcard pattern now. The GENIE reader opens the matching files and checks their GST tree when
        // conversion starts.
        if (getText("input").empty()) { throw std::runtime_error("--input GST ROOT file or glob is required"); }
        if (getText("event-generator") != "genie-gst") { throw std::runtime_error("Only --event-generator genie-gst is currently implemented"); }
        if ((getText("output-layout") != "nested") && (getText("output-layout") != "metadata")) { throw std::runtime_error("output-layout must be nested or metadata"); }

        // These values describe the physical input in the run directory and manifest. `unknown` and
        // `none` are meaningful recorded values, but empty text is not.
        for (auto k : {"event-generator-version", "tune", "q2-cut", "gemc-target-variation"}) {
            if (getText(k).empty()) { throw std::runtime_error(std::string(k) + " must not be empty"); }
        }

        // The remaining settings belong only to random uniform test events.
        return;
    }

    // The uniform LUND creator can create one electron, an electron with one hadron, or the electron test
    // scan. These random test events measure detector acceptance; they do not describe a physical
    // interaction.
    if ((getText("channel") != "1e") && (getText("channel") != "eh") && (getText("channel") != "electron-tester")) { throw std::runtime_error("channel must be 1e, eh or electron-tester"); }
    if ((getText("hadron") != "proton") && (getText("hadron") != "neutron") && (getText("hadron") != "pip") && (getText("hadron") != "pim")) {
        throw std::runtime_error("hadron must be proton, neutron, pip or pim");
    }
    if ((getText("hadron-region") != "FD") && (getText("hadron-region") != "CD")) { throw std::runtime_error("hadron-region must be FD or CD"); }

    // Theta is the angle measured from the beam direction. Require each minimum to be smaller than its
    // maximum and keep the full range between 0 and 180 degrees.
    for (auto stem : {"electron", "hadron"}) {
        double lo = getDouble(std::string(stem) + "-theta-min"), hi = getDouble(std::string(stem) + "-theta-max");
        if (!((0 <= lo) && (lo < hi) && (hi <= 180))) { throw std::runtime_error("Require 0 <= theta-min < theta-max <= 180"); }
    }

    // createFromCommandLine() has already replaced `auto` and the older spelling `sampled`. Mixed sampling chooses half
    // the values uniformly in momentum p and half uniformly in 1/p. Its minimum must be greater than zero
    // because 1/0 is undefined. Only neutron samples may use one fixed momentum.
    if ((getText("hadron-momentum") != "fixed") && (getText("hadron-momentum") != "uniform") && (getText("hadron-momentum") != "mixed")) {
        throw std::runtime_error("hadron-momentum must be fixed, sampled, uniform or mixed");
    }
    if ((getText("hadron-momentum") == "mixed") && ((getText("channel") != "eh") || (getText("hadron") == "neutron") || (getDouble("hadron-p-min") <= 0))) {
        throw std::runtime_error("mixed requires an eh charged hadron and strictly positive hadron-p-min");
    }
    if ((getText("hadron-momentum") == "fixed") && ((getText("channel") != "eh") || (getText("hadron") != "neutron"))) {
        throw std::runtime_error("fixed hadron momentum is available only for eh neutron samples");
    }

    if ((getText("electron-momentum") != "uniform") && (getText("electron-momentum") != "mixed") && (getText("electron-momentum") != "beam")) {
        throw std::runtime_error("electron-momentum must be auto, uniform, mixed or beam");
    }
    if ((getText("electron-momentum") == "mixed") && ((getText("channel") != "1e") || (getDouble("electron-p-min") <= 0))) {
        throw std::runtime_error("mixed electron momentum requires 1e and strictly positive electron-p-min");
    }

    // A fixed momentum must be greater than zero. A uniform range may begin at zero, but its maximum must
    // be greater than its minimum. The mixed-mode checks above require a minimum greater than zero.
    if ((getDouble("hadron-p") <= 0) || (getDouble("hadron-p-min") < 0) || (getDouble("beam-energy") <= getDouble("hadron-p-min"))) {
        throw std::runtime_error("Invalid hadron momentum bounds");
    }
    if ((getDouble("electron-p-min") < 0) || (getDouble("electron-p-max") <= getDouble("electron-p-min"))) { throw std::runtime_error("Invalid electron momentum bounds"); }

    // The trigger electron's theta must stay between 0 and 180 degrees. Its angle away from the opposite
    // detector sector must stay between -180 and 180 degrees.
    if ((getDouble("trigger-theta") < 0) || (getDouble("trigger-theta") > 180) || (std::abs(getDouble("trigger-phi-offset")) > 180)) { throw std::runtime_error("Invalid trigger angle"); }
#pragma endregion
}
#pragma endregion

// Preparing text for JSON -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Preparing text for JSON */
std::string quoteAsJsonString(const std::string& s) {
    // Build a new quoted string without changing the original text.
    std::ostringstream out;
    out << '"';

    // Read each byte as a value from 0 through 255 so checks for invisible control characters behave the
    // same on every platform.
    for (unsigned char ch : s) {
        // JSON uses quotes to mark a string and backslashes to begin special forms. Add a backslash before
        // either character when it is part of the text itself.
        if ((ch == '"') || (ch == '\\')) {
            out << '\\' << ch;
        } else if (ch < 0x20) {
            // JSON does not allow invisible control characters directly inside a string. Write them as
            // `\u` followed by four hexadecimal digits, then return number formatting to decimal.
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
        } else {
            // Visible text and bytes that form UTF-8 text can be copied unchanged.
            out << ch;
        }
    }

    // Add the closing quote so the result is ready to insert as one JSON key or value.
    out << '"';

    return out.str();
}
#pragma endregion

// Building command-line help --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Building command-line help */
std::string buildHelpText(LundSource source) {
    const bool uniform = (source == LundSource::Uniform);

    // Put quotes around the physical-input wildcard example so the shell passes `gst*.root` to the
    // program instead of replacing it before the program starts.
    std::string result = uniform ? "uniform-lund-creator --channel 1e|eh|electron-tester [--hadron proton|neutron|pip|pim --hadron-region FD|CD] --output PARENT_DIRECTORY\n"
                                 : "event-generator-to-lund-converter --event-generator genie-gst --input 'gst*.root' --output PARENT_DIRECTORY\n";

    // List the settings used by both random event creation and physical-input conversion.
    result +=
        "Settings: --config FILE, --beam-energy GeV, --target ID, --gemc-target-variation NAME, --A N, --Z N,\n"
        "--events N, --events-per-file N, --seed N, --vertex-seed N, --prefix NAME,\n"
        "Every event samples the selected target geometry.\n"
        "Files use key = value; CLI values override file settings. Seed 0 requests ROOT automatic, nonrepeatable seeding.\n"
        "Existing output is replaced after a warning.\n";
    if (uniform) { result += "Uniform event IDs start at zero and continue across split files.\n"; }

    if (uniform) {
        // List settings that control the particles and random ranges in uniform test events. createFromCommandLine()
        // calculates automatic values before checking them.
        result +=
            "Uniform: --hadron proton|neutron|pip|pim, --hadron-region FD|CD, --electron-theta-min/max DEG, --electron-p-min/max GeV/c,\n"
            "--hadron-theta-min/max DEG, --electron-momentum auto|uniform|mixed|beam,\n"
            "--hadron-momentum auto|fixed|sampled|uniform|mixed,\n"
            "--hadron-p GeV/c, --hadron-p-min GeV/c, --trigger-theta DEG, --trigger-phi-offset DEG.\n"
            "Hadron theta and phi are always sampled uniformly inside their configured ranges.\n"
            "Sampled hadron momentum extends to the beam energy. Uniform monitoring is always written and rendered.\n";
    } else {
        // List settings that describe existing GENIE input and the physical-output directory layout.
        result +=
            "Physical: --event-generator genie-gst (default), --event-generator-version VERSION, --tune NAME, --q2-cut NAME,\n"
            "--output-layout nested|metadata (default: nested).\n"
            "Tune defaults to TUNE from the production input_options.txt and falls back to unknown.\n"
            "For physical input, --events-per-file also sets the minimum inclusive input block required before a follow-up file starts, aligned with JOB_NEVENTS.\n";
    }

    // Ask the target catalog for its current names so this help text cannot contain an outdated copy.
    result += "Targets: " + targetNames() + "\n";

    return result;
}
#pragma endregion

}  // namespace samples
