//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file RunConfig.cpp
 * @brief Sample option parsing, validation and JSON escaping.
 *
 * Purpose:
 *   Implement the shared configuration boundary for uniform generation and physical conversion: keep
 *   invalid settings out of event processing and preserve the exact resolved configuration for the
 *   run manifest.
 *
 * Workflow:
 *   Defaults -> profile -> CLI overrides -> automatic values -> validation -> absolute paths.
 *
 * Accepted `--key value` options:
 *   Shared: config, beam-energy, rgm-target, target, A, Z, output, events, events-per-file,
 *           seed, vertex-seed, prefix, and gemc-target-variation.
 *   Uniform: channel, hadron, hadron-region, electron-theta-min/max, electron-p-min/max,
 *            electron-momentum, hadron-theta-min/max, hadron-p-min, hadron-p,
 *            hadron-momentum, trigger-theta, and trigger-phi-offset.
 *   Physical: input, event-generator, event-generator-version, tune, q2-cut, and gemc-version.
 *   `--help` is handled by each application before parse(); all other options require a following
 *   value. One optional profile uses the same names without the leading `--`.
 *
 * Precedence and defaults:
 *   CLI overrides profile values, which override the defaults installed here. Automatic values are
 *   resolved only after precedence is complete. The two application entry points document defaults,
 *   units, required values, and source-specific choices next to their user-facing usage.
 *
 * Boundary:
 *   This translation unit resolves configuration only. It performs no event generation/conversion,
 *   input-tree inspection, random sampling, output-directory replacement, LUND serialization,
 *   monitoring, or simulation submission. Downstream workflow objects perform those operations only
 *   after RunConfig::parse() returns successfully.
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
 * @brief Private text and naming helpers used only while resolving RunConfig values.
 *
 * These functions have internal linkage: they support profile parsing and deterministic output-name
 * construction without becoming part of the public configuration API.
 */
namespace {

// trim ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* trim */
/**
 * @brief Remove surrounding configuration whitespace.
 *
 * Purpose:
 *   Normalize profile lines, keys, and values while preserving whitespace inside the meaningful text.
 *
 * Algorithm:
 *   Find the first and last character outside the supported ASCII whitespace set and return the text
 *   between them. Return an empty string when no such character exists.
 *
 * @param s Configuration text copied into the helper so the caller's string is not modified.
 *
 * @return Text without leading/trailing spaces, tabs, carriage returns, or newlines; empty for an
 *         empty or whitespace-only input.
 *
 * @note Internal spaces and all other characters are retained exactly.
 */
std::string trim(std::string s) {
    // A missing first non-whitespace character covers both empty and whitespace-only strings.
    auto first = s.find_first_not_of(" \t\r\n");

    // Once `first` exists, a last non-whitespace character also exists and bounds the retained span.
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
#pragma endregion

// pathToken -------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* pathToken */
/**
 * @brief Convert explicit provenance text into one portable output-directory component.
 *
 * Purpose:
 *   Preserve recognizable target, generator, tune, cut, and GEMC metadata in a single path component
 *   while preventing separators, whitespace, and punctuation from changing the directory structure.
 *
 * Algorithm:
 *   Keep characters accepted by std::isalnum plus `.`, `_`, and `-`; replace every other byte with
 *   `-`; then reject components that are empty or have the special filesystem meanings `.` and `..`.
 *
 * @param value Unsanitized metadata copied from the fully resolved configuration.
 *
 * @return Sanitized component suitable for composition into an output directory name.
 *
 * @throws std::runtime_error If the resulting component is empty, `.` or `..`.
 *
 * @note Sanitization is deterministic but not reversible or collision-free. The manifest retains the
 *       original unsanitized metadata for provenance.
 */
std::string pathToken(std::string value) {
    // Cast through unsigned char before the cctype call to avoid undefined behavior for negative char
    // values. The three explicit punctuation characters are the only non-alphanumerics retained.
    for (char& ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '_' || ch == '-')) { ch = '-'; }
    }

    // Exclude empty and navigation-like names even though ordinary punctuation has been normalized.
    if (value.empty() || value == "." || value == "..") { throw std::runtime_error("Invalid empty output-name component"); }
    return value;
}
#pragma endregion

// beamMeV ---------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* beamMeV */
/**
 * @brief Convert beam energy in GeV to the established integer label in MeV.
 *
 * Purpose:
 *   Keep output names compatible with the RG-M 2070, 4029, and 5986 MeV conventions while still
 *   supporting other configured energies through a deterministic rounded label.
 *
 * Algorithm:
 *   Match each established GeV setting within `1e-6` GeV and return its fixed label. Otherwise,
 *   multiply by 1000 and round to the nearest integer using std::llround.
 *
 * @param energy Finite beam energy in GeV, obtained through RunConfig::number().
 *
 * @return Beam-energy label in MeV for filenames and directory names.
 *
 * @note The fixed 2.07052 GeV mapping intentionally yields the historical `2070` label rather than
 *       the generic rounded value `2071`.
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
 * @brief Build the public uniform-sample label from resolved particle and detector selections.
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
/**
 * @brief Resolve one complete sample configuration.
 *
 * Purpose:
 *   Make both executables use the same precedence, automatic-resolution, naming, and validation
 *   contract before any workflow creates outputs.
 *
 * Workflow:
 *   1. Install shared defaults plus exactly one source-specific key set.
 *   2. Parse strict `--key value` CLI pairs, retaining them as final-precedence overrides.
 *   3. Read one optional `key = value` profile over the defaults.
 *   4. Apply CLI overrides and resolve target-, channel-, and beam-dependent `auto` values.
 *   5. Validate the complete scientific and output contract before filesystem mutation is possible.
 *   6. Build the source-specific run-directory name and normalize input/output paths.
 *
 * @param argc Number of argv entries, including the executable name.
 * @param argv Borrowed CLI tokens read during this call; their storage is neither retained nor changed.
 * @param uniform Select uniform sampling when true or physical event conversion when false. This
 *                choice defines the accepted keys, defaults, automatic values, and validation branch.
 *
 * @return Owning, fully resolved configuration. Local paths are absolute and lexically normalized;
 *         the output value names the final source-specific run directory beneath the requested parent.
 *
 * @throws std::exception For malformed or repeated input, unknown keys, unreadable profiles, invalid
 *         target/source metadata, invalid numeric or physical bounds, and path conversion failures.
 *
 * @note This function reads arguments and an optional profile, then computes paths and names. It does
 *       not inspect physical event input or mutate the output filesystem. Downstream workflows report
 *       and safely replace the resolved output directory immediately before writing a run.
 */
RunConfig RunConfig::parse(int argc, char** argv, bool uniform) {
#pragma region /* Default settings */
    // Store all settings as text so the exact resolved values used by generation can also be written
    // to provenance. Shared defaults preserve the established RG-M beam/Ar setup, separate vertex and
    // kinematic seeds and the single supported LUND text contract. Particle masses are owned by the
    // protected target source and are not configurable here.
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

    // Add only the keys meaningful to the selected source. This makes a physical-only option invalid
    // for uniform generation and vice versa instead of silently accepting an unused setting.
    if (uniform) {
        // Angles are degrees and momenta are GeV/c. Channel-dependent `auto` values are resolved only
        // after profile and CLI precedence is complete. Fixed 1 GeV/c momentum is neutron-only.
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
        // Physical mode describes existing event-generator truth and its output provenance. It does
        // not run GENIE; the current adapter reads GENIE GST input selected below by event-generator.
        c.values_.insert({{"input", ""}, {"event-generator", "genie-gst"}, {"event-generator-version", "unknown"}, {"tune", "unknown"}, {"q2-cut", "auto"}, {"gemc-version", "unknown"}});
    }

    // Centralize assignment so profiles and CLI overrides share the same strict known-key policy.
    // Capturing `c` by reference mutates only the configuration being constructed in this call.
    auto assign = [&](const std::string& k, const std::string& v) {
        if (!c.values_.count(k)) { throw std::runtime_error("Unknown setting: " + k); }

        c.values_[k] = v;
    };
#pragma endregion

#pragma region /* Profile and CLI input */
    // Collect CLI overrides separately so they always apply after the profile, regardless of where
    // `--config` appears in argv. Maps also provide deterministic duplicate detection and traversal.
    std::map<std::string, std::string> overrides;
    std::string config;

    // Every application option is a two-token `--key value` pair. Booleans therefore use explicit
    // text values, and negative numbers remain ordinary value tokens consumed with their key.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--", 0) != 0 || i + 1 == argc) { throw std::runtime_error("Expected --key value: " + arg); }

        auto key = arg.substr(2);
        std::string value = argv[++i];

        // `config` selects the profile itself and is not a stored run setting or manifest field.
        if (key == "config") {
            if (!config.empty()) { throw std::runtime_error("Use only one --config file"); }

            config = value;
        } else {
            if (!overrides.emplace(key, value).second) { throw std::runtime_error("Repeated option: " + key); }
        }
    }

    // Read one profile, rejecting duplicate keys instead of silently replacing values. Relative
    // profile paths follow the executable's current working directory, which workflow.py anchors to
    // the repository root for maintained launches.
    if (!config.empty()) {
        std::ifstream in(config);

        if (!in) { throw std::runtime_error("Cannot open config: " + config); }

        std::string line;
        std::map<std::string, bool> seen;

        while (std::getline(in, line)) {
            line = trim(line);

            // Empty lines and full-line comments are ignored. Inline `#` remains part of the value.
            if (line.empty() || line[0] == '#') { continue; }

            // Split at the first equals sign so later equals characters remain available in the value.
            auto eq = line.find('=');

            if (eq == std::string::npos) { throw std::runtime_error("Expected key = value: " + line); }

            auto key = trim(line.substr(0, eq));

            if (seen[key]) { throw std::runtime_error("Repeated config key: " + key); }

            seen[key] = true;

            // Assignment checks the source-specific key vocabulary; value semantics are validated only
            // after CLI overrides and automatic-value resolution are complete.
            assign(key, trim(line.substr(eq + 1)));
        }
    }
#pragma endregion

#pragma region /* Automatic-value resolution */
    // Apply explicit options last. A CLI value may itself be `auto`, in which case the resolution below
    // treats that as an explicit request to recompute the context-dependent value.
    for (const auto& [k, v] : overrides) { assign(k, v); }

    // Resolve the complete default target definition first, analogous to the legacy target-selection
    // block but centralized for every supported RG-M identity. Preserve the four pre-resolution values
    // so an explicitly configured geometry, A/Z pair, or GEMC variation can overwrite its catalog
    // default only after the coherent identity defaults have been installed.
    const std::string target_override = c.get("target");
    const std::string A_override = c.get("A");
    const std::string Z_override = c.get("Z");
    const std::string variation_override = c.get("gemc-target-variation");
    const auto& rgm_target = findRgmTarget(c.get("rgm-target"));
    c.values_["target"] = rgm_target.geometry;
    c.values_["A"] = std::to_string(rgm_target.A);
    c.values_["Z"] = std::to_string(rgm_target.Z);
    c.values_["gemc-target-variation"] = rgm_target.gemc_variation;

    // Independent overrides are deliberately applied last. Normal profiles therefore need only
    // `rgm-target`, while compatibility studies may replace any one field without changing identity.
    if (target_override != "auto") { c.values_["target"] = target_override; }
    if (A_override != "auto") { c.values_["A"] = A_override; }
    if (Z_override != "auto") { c.values_["Z"] = Z_override; }
    if (variation_override != "auto") { c.values_["gemc-target-variation"] = variation_override; }

    if (uniform) {
        // Resolve the species/region acceptance contract. CD pions stop at 140 degrees; CD nucleons
        // stop at 145. FD neutrons stop at 35, while charged FD hadrons stop at 45 degrees.
        const bool neutron = c.get("hadron") == "neutron";
        const bool pion = c.get("hadron") == "pip" || c.get("hadron") == "pim";
        const bool central = c.get("hadron-region") == "CD";
        if (c.get("hadron-theta-min") == "auto") { c.values_["hadron-theta-min"] = central ? "35" : "5"; }
        if (c.get("hadron-theta-max") == "auto") { c.values_["hadron-theta-max"] = central ? (pion ? "140" : "145") : (neutron ? "35" : "45"); }
        if (c.get("electron-p-max") == "auto") { c.values_["electron-p-max"] = c.get("beam-energy"); }
        if (c.get("hadron-p-min") == "auto") { c.values_["hadron-p-min"] = neutron ? "0" : c.get("hadron") == "proton" ? (central ? "0.2" : "0.3") : (central ? "0.1" : "0.2"); }

        // Trigger-electron sector offsets are legacy beam-setting values in degrees. Unknown beam
        // energies receive zero offset rather than an inferred experimental configuration.
        if (c.get("trigger-phi-offset") == "auto") {
            double e = c.number("beam-energy");
            c.values_["trigger-phi-offset"] = std::abs(e - 2.07052) < 1e-6 ? "16" : std::abs(e - 4.02962) < 1e-6 ? "7" : std::abs(e - 5.98636) < 1e-6 ? "5" : "0";
        }
    }

    // Physical directory provenance uses the established analysis Q2-cut label for each RG-M beam;
    // other energies explicitly record that no known automatic label applies.
    if (!uniform && c.get("q2-cut") == "auto") {
        const double e = c.number("beam-energy");
        c.values_["q2-cut"] = std::abs(e - 2.07052) < 1e-6 ? "Q2_0_02" : std::abs(e - 4.02962) < 1e-6 ? "Q2_0_25" : std::abs(e - 5.98636) < 1e-6 ? "Q2_0_40" : "none";
    }

    // The file prefix stays concise and recognizable. Physical run-directory naming below retains the
    // fuller target-variation, generator-version, tune, Q2-cut, beam, and GEMC provenance contract.
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
        // Resolve production momentum by particle: charged hadrons use the 50/50 p and 1/p mixture;
        // neutrons use uniform p. Every eh trigger electron remains fixed at beam momentum.
        if (c.get("electron-momentum") == "auto") { c.values_["electron-momentum"] = c.get("channel") == "1e" ? "mixed" : "beam"; }
        if (c.get("hadron-momentum") == "auto" || c.get("hadron-momentum") == "sampled") {
            c.values_["hadron-momentum"] = c.get("channel") == "eh" && c.get("hadron") != "neutron" ? "mixed" : "uniform";
        }
    }
#pragma endregion

#pragma region /* Validation and path resolution */
    // Validate while `output` still names the requested parent and local `input` retains the supplied
    // spelling. Validation performs no output creation or replacement.
    c.validate(uniform);

    // Preserve URI-like physical inputs for adapters that understand them. Normalize local files and
    // glob patterns against the current directory so downstream behavior is independent of later cwd.
    if (!uniform && c.get("input").find("://") == std::string::npos) { c.values_["input"] = std::filesystem::absolute(c.get("input")).lexically_normal().string(); }

    if (uniform) {
        // Uniform output keeps the legacy recognizable channel/beam directory below the user-selected
        // parent. Width four provides labels such as 2070, 4029, and 5986 without truncating others.
        std::ostringstream directory;
        directory << "Uniform_sample_" << uniformSampleLabel(c) << '_' << std::setw(4) << std::setfill('0') << beamMeV(c.number("beam-energy")) << "MeV";
        c.values_["output"] = (std::filesystem::path(c.get("output")) / directory.str()).string();
    } else {
        // Physical output encodes explicit simulation provenance in the documented order. Each field
        // is sanitized only for this path; its unsanitized resolved value remains in values_ and is
        // therefore available to the manifest writer.
        std::ostringstream directory;
        directory << pathToken(c.get("gemc-target-variation")) << "__" << pathToken(c.get("event-generator")) << '-' << pathToken(c.get("event-generator-version")) << "__"
                  << pathToken(c.get("tune")) << "__" << pathToken(c.get("q2-cut")) << "__" << beamMeV(c.number("beam-energy")) << "MeV_GEMC-" << pathToken(c.get("gemc-version"));
        c.values_["output"] = (std::filesystem::path(c.get("output")) / directory.str()).string();
    }

    // Expose one absolute, lexically normalized final run directory to every downstream consumer.
    // Filesystem creation and guarded replacement remain the generator/writer's responsibility.
    c.values_["output"] = std::filesystem::absolute(c.get("output")).lexically_normal().string();
#pragma endregion

    return c;
}
#pragma endregion

// RunConfig::get --------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::get */
/**
 * @brief Read a resolved setting as text.
 *
 * Purpose:
 *   Give generators and provenance writers a read-only copy of one value without exposing mutable
 *   access to the configuration map.
 *
 * @param k Known shared or selected-source configuration key.
 *
 * @return Copy of the stored resolved string, preserving its manifest representation.
 *
 * @throws std::out_of_range If the key does not exist in this configuration's source-specific map.
 */
std::string RunConfig::get(const std::string& k) const {
    const auto value = values_.find(k);
    if (value == values_.end()) { throw std::out_of_range("Missing configuration key: " + k); }
    return value->second;
}
#pragma endregion

// RunConfig::number -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::number */
/**
 * @brief Read a finite floating-point setting.
 *
 * Purpose:
 *   Convert a resolved numeric setting at the point of use while retaining its original string for
 *   provenance. The key's contract supplies the unit, such as GeV/c, GeV, degrees, or cm.
 *
 * Algorithm:
 *   Use std::stod while recording the consumed character count, then require the entire string to be
 *   consumed and the result to be finite.
 *
 * @param k Key whose resolved value must represent one floating-point number.
 *
 * @return Finite double in the unit documented for the selected key.
 *
 * @throws std::out_of_range If the key is absent or the number exceeds double range.
 * @throws std::invalid_argument If std::stod cannot begin a conversion.
 * @throws std::runtime_error If trailing text remains or the converted value is non-finite.
 */
double RunConfig::number(const std::string& k) const {
    // `used` distinguishes a complete value such as "5.98636" from a numeric prefix such as "5 GeV".
    std::size_t used = 0;
    double value = std::stod(get(k), &used);

    // NaN and infinity are syntactically accepted by stod on some libraries but are never valid run
    // configuration values, so reject them explicitly along with partially parsed strings.
    if (used != get(k).size() || !std::isfinite(value)) { throw std::runtime_error("Invalid number: " + k); }

    return value;
}
#pragma endregion

// RunConfig::integer ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::integer */
/**
 * @brief Read an unsigned integer setting.
 *
 * Purpose:
 *   Parse counts, seeds, and nuclear metadata without accepting signs, whitespace, fractional text,
 *   or implementation-dependent integer prefixes.
 *
 * Algorithm:
 *   Copy the resolved string, require at least one ASCII decimal digit and no other character, then
 *   convert it with std::stoull's range checking.
 *
 * @param k Key whose resolved value must contain unsigned decimal digits only.
 *
 * @return Parsed 64-bit unsigned integer. Per-key limits are enforced separately by validate().
 *
 * @throws std::out_of_range If the key is absent or the digits exceed std::uint64_t range.
 * @throws std::runtime_error If the stored value is empty or contains a non-digit.
 */
std::uint64_t RunConfig::integer(const std::string& k) const {
    // Validate spelling before conversion so signs and surrounding whitespace cannot be accepted by
    // stoull even when they would otherwise produce a numeric result.
    const auto s = get(k);

    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) { throw std::runtime_error("Expected unsigned integer: " + k); }

    // stoull supplies the final overflow check after the lexical digit-only contract is satisfied.
    return std::stoull(s);
}
#pragma endregion

// RunConfig::validate ---------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* RunConfig::validate */
/**
 * @brief Reject incompatible or invalid run settings before output creation.
 *
 * Purpose:
 *   Establish the complete configuration invariant relied upon by event sources, target-vertex
 *   sampling, LUND serialization, monitoring, naming, and provenance. Validation is deliberately
 *   read-only and runs before a generator may replace or create the resolved output directory.
 *
 * Workflow:
 *   1. Check required shared values, beam energy, counts/seeds, nuclear metadata, and filename prefix.
 *   2. Validate the external target-geometry key used by every event.
 *   3. For physical conversion, require current GENIE GST input and complete naming provenance.
 *   4. For uniform generation, validate the channel, angular acceptance, momentum modes,
 *      momentum bounds, and trigger-electron angles.
 *
 * @param uniform Select uniform-generation constraints when true or physical-conversion constraints
 *                when false. It must match the source mode used by parse().
 *
 * @return Nothing. Normal return means every applicable check succeeded.
 *
 * @throws std::exception If a key is missing, a stored numeric value cannot be converted, a target
 *         geometry is unknown, or any shared/source-specific constraint fails.
 *
 * @note Numeric units follow the configuration contract: beam energy is GeV, momentum is GeV/c, angles
 *       are degrees, and sampled target coordinates are cm.
 */
void RunConfig::validate(bool uniform) const {
#pragma region /* Shared run and output contract */
    // Output is still the caller-selected parent at this stage; parse() appends the final run name only
    // after validation. Events is mandatory for both generated and converted LUND workflows.
    if (get("output").empty()) { throw std::runtime_error("--output is required; use a new run directory"); }
    if (get("events").empty()) { throw std::runtime_error("--events is required; provide the total number of events to write"); }

    // Beam energy must be a finite, strictly positive GeV value. number() owns lexical/finite checks.
    if (number("beam-energy") <= 0) { throw std::runtime_error("beam-energy must be positive"); }

    // Total capacity and split size must be positive. RNG seeds additionally allow zero because ROOT
    // defines TRandom3(0) as a request for automatic, nonrepeatable seeding; users may select that
    // behavior deliberately even though it prevents exact event reproduction from the manifest alone.
    for (auto k : {"events", "events-per-file"}) {
        auto n = integer(k);
        if (!n || n > std::numeric_limits<unsigned int>::max()) { throw std::runtime_error(std::string(k) + " must be in [1, 4294967295]"); }
    }
    for (auto k : {"seed", "vertex-seed"}) {
        if (integer(k) > std::numeric_limits<unsigned int>::max()) { throw std::runtime_error(std::string(k) + " must be in [0, 4294967295]"); }
    }

    // A and Z are LUND header metadata, independently configurable from target geometry. integer()
    // already enforces Z >= 0; these relationships keep the requested nuclide internally consistent.
    if (integer("A") < 1 || integer("A") > 300 || integer("Z") > integer("A")) { throw std::runtime_error("Require 1 <= A <= 300 and 0 <= Z <= A"); }

    // Prefix becomes part of every output filename, so accept one nonempty portable component only.
    if (get("prefix").empty() || get("prefix").find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos) {
        throw std::runtime_error("prefix must contain only letters, numbers, _, . or -");
    }

#pragma endregion

#pragma region /* Vertex contract */
    // Every maintained event samples the selected target geometry. There is no fixed-vertex mode.
    TargetGeometry::validate(get("target"));
#pragma endregion

#pragma region /* Source-specific contract */
    if (!uniform) {
        // Physical mode currently has one adapter: GENIE GST ROOT input. A nonempty path/glob is
        // required here; adapter opening and tree/schema checks occur when conversion begins.
        if (get("input").empty()) { throw std::runtime_error("--input GST ROOT file or glob is required"); }
        if (get("event-generator") != "genie-gst") { throw std::runtime_error("Only --event-generator genie-gst is currently implemented"); }

        // These fields form the physical output-directory contract and manifest provenance. Tokens
        // such as `unknown` or `none` remain explicit valid values; omission is not allowed.
        for (auto k : {"event-generator-version", "tune", "q2-cut", "gemc-version", "gemc-target-variation"}) {
            if (get(k).empty()) { throw std::runtime_error(std::string(k) + " must not be empty"); }
        }

        // Physical configurations do not contain uniform-only keys, so finish after their own branch.
        return;
    }

    // Uniform source selection fixes event multiplicity/content to electron-only, electron-proton, or
    // electron-neutron generation; it never represents a physical interaction model.
    if (get("channel") != "1e" && get("channel") != "eh" && get("channel") != "electron-tester") { throw std::runtime_error("channel must be 1e, eh or electron-tester"); }
    if (get("hadron") != "proton" && get("hadron") != "neutron" && get("hadron") != "pip" && get("hadron") != "pim") {
        throw std::runtime_error("hadron must be proton, neutron, pip or pim");
    }
    if (get("hadron-region") != "FD" && get("hadron-region") != "CD") { throw std::runtime_error("hadron-region must be FD or CD"); }

    // Require ordered polar-angle bounds inside the full geometric domain. The resolved defaults carry
    // the legacy per-particle acceptance, while explicit profiles may narrow those ranges.
    for (auto stem : {"electron", "hadron"}) {
        double lo = number(std::string(stem) + "-theta-min"), hi = number(std::string(stem) + "-theta-max");
        if (!(0 <= lo && lo < hi && hi <= 180)) { throw std::runtime_error("Require 0 <= theta-min < theta-max <= 180"); }
    }

    // parse() resolves `auto`/`sampled` before this function. Mixed sampling requires a positive lower
    // bound because inverse momentum is undefined at zero. Fixed momentum is intentionally neutron-only.
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

    // The fixed momentum must be positive. Sampled bounds allow zero for uniform sampling, require a
    // positive-width interval, and receive the stricter positive minimum above for inverse-p mixing.
    if (number("hadron-p") <= 0 || number("hadron-p-min") < 0 || number("beam-energy") <= number("hadron-p-min")) { throw std::runtime_error("Invalid hadron momentum bounds"); }
    if (number("electron-p-min") < 0 || number("electron-p-max") <= number("electron-p-min")) { throw std::runtime_error("Invalid electron momentum bounds"); }

    // Trigger theta is a polar angle and the signed sector-offset magnitude cannot exceed 180 degrees.
    if (number("trigger-theta") < 0 || number("trigger-theta") > 180 || std::abs(number("trigger-phi-offset")) > 180) { throw std::runtime_error("Invalid trigger angle"); }
#pragma endregion
}
#pragma endregion

// jsonString ------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* jsonString */
/**
 * @brief Encode a string as a JSON string literal.
 *
 * Purpose:
 *   Serialize resolved configuration and provenance text into manifests without allowing quotes,
 *   backslashes, or control bytes to break the surrounding strict JSON document.
 *
 * Algorithm:
 *   1. Write an opening double quote.
 *   2. Prefix `"` and `\` bytes with a backslash.
 *   3. Encode bytes below U+0020 as four-digit `\u00XX` escapes.
 *   4. Copy other bytes unchanged and write the closing quote.
 *
 * @param s Borrowed, unescaped configuration or provenance text; it is not modified or retained.
 *
 * @return Complete JSON string literal including its surrounding double quotes.
 *
 * @note Bytes at or above 0x20 are preserved. Callers therefore retain UTF-8 text byte-for-byte and
 *       are responsible for supplying valid text encoding when the manifest will contain Unicode.
 *
 * @note This helper returns encoded text only; it does not write a file or validate a complete JSON
 *       object. The manifest writer owns document structure and output I/O.
 */
std::string jsonString(const std::string& s) {
    // Build an independent result so the caller's source string remains available in RunConfig.
    std::ostringstream out;
    out << '"';

    // Iterate as unsigned bytes: control-byte comparisons must not depend on whether plain char is
    // signed on the current platform.
    for (unsigned char ch : s) {
        // Quotes could terminate the literal and backslashes begin JSON escapes, so preserve each as a
        // two-byte escaped sequence.
        if (ch == '"' || ch == '\\') {
            out << '\\' << ch;
        } else if (ch < 0x20) {
            // JSON forbids raw control bytes. Emit the equivalent zero-padded Unicode escape and then
            // restore decimal stream formatting for subsequent output.
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
        } else {
            // Ordinary ASCII and multibyte UTF-8 bytes require no additional JSON escaping here.
            out << ch;
        }
    }

    // The returned value is ready to insert as one JSON key or value, including its delimiters.
    out << '"';

    return out.str();
}
#pragma endregion

// help ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* help */
/**
 * @brief Describe CLI options for the selected application.
 *
 * Purpose:
 *   Keep command-line guidance adjacent to the parser contract so the uniform generator and physical
 *   converter describe the same shared configuration grammar, units, and output-replacement behavior.
 *
 * Workflow:
 *   Start with the selected executable's usage line, append options shared by both LUND sources, add
 *   the matching source-specific controls, and finish with target identifiers from the maintained
 *   RG-M target table.
 *
 * @param uniform Select `uniform-lund-generator` help when true or `event-generator-to-lund-converter` help when false.
 *
 * @return Newly owned multiline usage text. The caller decides where to print it.
 *
 * @note This function performs no parsing, file access, generation, conversion, or submission. Target
 *       names come from rgmTargetNames(), keeping help synchronized with the accepted identity table.
 */
std::string help(bool uniform) {
    // The physical example quotes its input glob so an interactive shell passes the pattern to the
    // converter instead of expanding it before the adapter receives it.
    std::string result = uniform ? "uniform-lund-generator --channel 1e|eh|electron-tester [--hadron proton|neutron|pip|pim --hadron-region FD|CD] --output PARENT_DIRECTORY\n"
                                 : "event-generator-to-lund-converter --event-generator genie-gst --input 'gst*.root' --output PARENT_DIRECTORY\n";

    // Shared settings control beam/target metadata, event and RNG counts, and automatic naming.
    result +=
        "Settings: --config FILE, --beam-energy GeV, --rgm-target ID, --target GEOMETRY, --A N, --Z N,\n"
        "--events N, --events-per-file N, --seed N, --vertex-seed N, --prefix NAME,\n"
        "Every event samples the selected target geometry.\n"
        "Files use key = value; CLI values override file settings. Seed 0 requests ROOT automatic, nonrepeatable seeding.\n"
        "Existing output is replaced after a warning.\n";

    if (uniform) {
        // Uniform-only settings select acceptance ranges and deliberately unphysical momentum/angle
        // sampling. Automatic aliases are resolved by RunConfig::parse before validation.
        result +=
            "Uniform: --hadron proton|neutron|pip|pim, --hadron-region FD|CD, --electron-theta-min/max DEG, --electron-p-min/max GeV/c,\n"
            "--hadron-theta-min/max DEG, --electron-momentum auto|uniform|mixed|beam,\n"
            "--hadron-momentum auto|fixed|sampled|uniform|mixed,\n"
            "--hadron-p GeV/c, --hadron-p-min GeV/c, --trigger-theta DEG, --trigger-phi-offset DEG.\n"
            "Hadron theta and phi are always sampled uniformly inside their configured ranges.\n"
            "Sampled hadron momentum extends to the beam energy. Uniform monitoring is always written and rendered.\n";
    } else {
        // Physical-only settings identify the current GENIE GST adapter and preserve generator, tune,
        // selection, detector, and target-variation provenance in the output contract.
        result +=
            "Physical: --event-generator genie-gst (default), --event-generator-version VERSION, --tune NAME,\n"
            "--q2-cut NAME, --gemc-version VERSION, --gemc-target-variation NAME.\n"
            "For physical input, --events-per-file also sets the minimum inclusive input block required before a follow-up file starts, aligned with JOB_NEVENTS.\n";
    }

    // Query the authoritative maintained identity table instead of duplicating its supported names.
    result += "RG-M targets: " + rgmTargetNames() + "\n";

    return result;
}
#pragma endregion

}  // namespace samples
