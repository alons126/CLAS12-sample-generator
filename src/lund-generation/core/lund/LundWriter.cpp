//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.cpp
 * @brief Implements LUND file writing and the completed-run log.
 *
 * Purpose:
 *   Print run settings, safely replace the requested run directory, write split LUND files, count every
 *   successful event, and record the final settings, build information, and output files.
 *
 * Workflow:
 *   Print the setup -> check and replace the exact run directory -> open or rotate LUND files as needed
 *   -> write headers and particles in order -> let the source save monitoring -> close the active file
 *   -> write lund-gen-log.json.tmp -> rename it to lund-gen-log.json.
 *
 * Written format:
 *   The LUND format uses fixed whitespace, precision, and uniform per-file IDs. Particle masses come
 *   from the external target source through particleMass(). Records use momentum in GeV/c, mass in
 *   GeV/c², energy in GeV, and vertices in cm.
 *
 * Failure:
 *   Unsafe replacement targets are rejected before deletion. Stream and filesystem failures throw and
 *   may leave partial output for inspection; absence of lund-gen-log.json marks the run incomplete.
 */

#include "core/lund/LundWriter.h"

#include <TROOT.h>
#include <TString.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "Version.h"
#include "support/environment.h"

namespace env = environment;

namespace samples {

// LundWriter::printWorkflowSummary --------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::printWorkflowSummary */
void LundWriter::printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned, std::uint64_t written, bool final) {
    // Only `uniform` uses the uniform settings and paths. Building these path values creates no files.
    const bool uniform = workflow == "uniform";
    const auto output = std::filesystem::path(config.get("output"));
    const auto lund_dir = output / "lundfiles";
    const auto diagnostics = output / "lundfiles" / "lund-gen-monitoring";

    // Print every label and value in the same one-line form.
    const auto print_value = [](const std::string& label, const auto& value) { std::cout << env::SYSTEM_COLOR << label << ":" << env::RESET_COLOR << " " << value << "\n"; };

    // Make setup and completion easy to find in long batch logs.
    std::cout << env::SYSTEM_COLOR << "\n=============================================================\n"
              << "= " << (uniform ? "Uniform sample generation" : "Physical generator to LUND conversion") << (final ? " completion\n" : " setup\n")
              << "=============================================================\n"
              << env::RESET_COLOR;

    // Completion reports do not repeat setup values.
    if (final) {
        const auto events_per_file = config.integer("events-per-file");
        const auto output_files = (written + events_per_file - 1) / events_per_file;

        std::cout << env::SYSTEM_COLOR << "\n- Event counts ----------------------------------------------\n" << env::RESET_COLOR;
        print_value(uniform ? "Events generated" : "Input entries scanned", scanned);
        print_value("Events written", written);
        print_value("LUND files written", output_files);
        print_value("Status", "complete");
        std::cout << "\n";

        return;
    }

    // These limits control the total count and file splitting for both sources.
    std::cout << env::SYSTEM_COLOR << "\n- Run limits ------------------------------------------------\n" << env::RESET_COLOR;
    print_value("Requested events", config.get("events"));
    print_value("Events per file", config.get("events-per-file"));

    // Beam and target settings control event headers and vertex sampling. Geometry and A/Z stay separate.
    std::cout << env::SYSTEM_COLOR << "\n- Beam and target -------------------------------------------\n" << env::RESET_COLOR;
    print_value("Beam energy [GeV]", config.get("beam-energy"));
    print_value("RG-M target", config.get("rgm-target"));
    print_value("Target geometry", config.get("target"));
    print_value("Target A", config.get("A"));
    print_value("Target Z", config.get("Z"));
    print_value("Vertex seed", config.get("vertex-seed") + (config.get("vertex-seed") == "0" ? " (ROOT automatic; nonrepeatable)" : ""));

    if (uniform) {
        const auto channel = config.get("channel");
        const bool electron_hadron = channel == "eh";
        const bool electron_tester = channel == "electron-tester";

        // Print only the kinematic settings used by the selected channel.
        std::cout << env::SYSTEM_COLOR << "\n- Uniform event content -------------------------------------\n" << env::RESET_COLOR;
        print_value("Channel", channel);
        print_value("Kinematic seed", config.get("seed") + (config.get("seed") == "0" ? " (ROOT automatic; nonrepeatable)" : ""));

        if (electron_hadron) {
            print_value("Hadron", config.get("hadron"));
            print_value("Hadron region", config.get("hadron-region"));
            print_value("Hadron momentum mode", config.get("hadron-momentum"));
            print_value("Hadron theta minimum [deg]", config.get("hadron-theta-min"));
            print_value("Hadron theta maximum [deg]", config.get("hadron-theta-max"));

            if (config.get("hadron-momentum") == "fixed") {
                print_value("Hadron momentum [GeV/c]", config.get("hadron-p"));
            } else {
                print_value("Hadron momentum minimum [GeV/c]", config.get("hadron-p-min"));
                print_value("Hadron momentum maximum [GeV/c]", config.get("beam-energy"));
            }

            print_value("Trigger-electron theta [deg]", config.get("trigger-theta"));
            print_value("Trigger-electron phi offset [deg]", config.get("trigger-phi-offset"));
        } else {
            print_value("Electron momentum mode", config.get("electron-momentum"));
            print_value("Electron theta minimum [deg]", config.get("electron-theta-min"));
            print_value("Electron theta maximum [deg]", config.get("electron-theta-max"));

            if (!electron_tester) {
                print_value("Electron momentum minimum [GeV/c]", config.get("electron-p-min"));
                print_value("Electron momentum maximum [GeV/c]", config.get("electron-p-max"));
            }
        }
    } else {
        // These values identify the physical input in names and the run log.
        std::cout << env::SYSTEM_COLOR << "\n- Physical input --------------------------------------------\n" << env::RESET_COLOR;
        print_value("Event generator", config.get("event-generator"));
        print_value("Event generator version", config.get("event-generator-version"));
        print_value("Input files", config.get("input"));
        print_value("Generator tune", config.get("tune"));
        print_value("Q2-cut label", config.get("q2-cut"));
        print_value("GEMC version", config.get("gemc-version"));
        print_value("GEMC target variation", config.get("gemc-target-variation"));
    }

    // Print only paths that the selected workflow actually creates or consumes.
    std::cout << env::SYSTEM_COLOR << "\n- Output ----------------------------------------------------\n" << env::RESET_COLOR;
    print_value("Output prefix", config.get("prefix"));
    print_value("Run directory", output);
    print_value("LUND directory", lund_dir);
    print_value("Completion manifest", diagnostics / "lund-gen-log.json");

    if (uniform) {
        print_value("MC HIPO directory", output / "mchipo");
        print_value("Reconstructed HIPO directory", output / "reconhipo");
        print_value("Monitoring ROOT file", diagnostics / (config.get("prefix") + "_monitoring_plots.root"));
        print_value("Monitoring plot directory", diagnostics / "MonitoringPlotsPath");
    }

    std::cout << "\n";
}
#pragma endregion

// LundWriter::LundWriter ------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::LundWriter */
LundWriter::LundWriter(const RunConfig& c, std::string workflow)
    : config_(c), workflow_(std::move(workflow)), directory_(c.get("output")), events_per_file_(c.integer("events-per-file")), capacity_(c.integer("events")) {
    // Check the path again immediately before code may delete an existing directory.
    directory_ = std::filesystem::absolute(directory_).lexically_normal();

    // Read the paths that must never be deleted. HOME may be missing in some batch environments.
    const auto root = directory_.root_path();
    const auto source = std::filesystem::path(SAMPLE_SOURCE_DIR).lexically_normal();
    const char* home_value = std::getenv("HOME");
    const auto home = home_value ? std::filesystem::path(home_value).lexically_normal() : std::filesystem::path();

    // Add a path separator so `/work/run` matches `/work/run/source` but not `/work/run-old`.
    const auto directory_text = directory_.string() + std::filesystem::path::preferred_separator;
    const bool contains_checkout = source == directory_ || source.string().rfind(directory_text, 0) == 0;

    // Reject any path that could erase a root, home, working directory, or source checkout.
    if (directory_.empty() || directory_ == root || (!home.empty() && directory_ == home) || directory_.filename().empty() || directory_ == std::filesystem::current_path() ||
        contains_checkout) {
        throw std::runtime_error("Refusing unsafe output-directory replacement: " + directory_.string());
    }

    // Create the parent, then replace only the exact final run directory when it already exists.
    std::filesystem::create_directories(directory_.parent_path());
    if (std::filesystem::exists(directory_)) {
        std::cout << env::WARNING_COLOR << "Replacing existing run directory (legacy behavior): " << directory_ << env::RESET_COLOR << "\n";
        std::filesystem::remove_all(directory_);
    }

    // write() opens the first LUND file, so construction cannot create an empty numbered file.
    std::filesystem::create_directories(directory_ / "lundfiles" / "lund-gen-monitoring");

    // Uniform runs prepare the folders later used by simulation, reconstruction, and plot output.
    if (workflow_ == "uniform") {
        for (const auto* child : {"mchipo", "reconhipo"}) { std::filesystem::create_directories(directory_ / child); }
        std::filesystem::create_directories(directory_ / "lundfiles" / "lund-gen-monitoring" / "MonitoringPlotsPath");
    }
}
#pragma endregion

// LundWriter::full ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::full */
bool LundWriter::full() const { return count_ >= capacity_; }
#pragma endregion

// LundWriter::write -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::write */
void LundWriter::write(const Event& e) {
    // Check the limit here even when the caller already used full(). A LUND event must have a particle.
    if (full()) { throw std::runtime_error("Run file limit reached"); }
    if (e.particles.empty()) { throw std::runtime_error("Cannot write an empty event"); }

    // Open a file only when an event needs it. This avoids an empty final file.
    if (files_.empty() || files_.back().events == events_per_file_) {
        // Close and flush the previous file before opening the next one.
        if (stream_.is_open()) { stream_.close(); }

        // Number files from one and store paths relative to the run directory.
        files_.push_back({"lundfiles/" + config_.get("prefix") + "_" + std::to_string(files_.size() + 1) + ".txt", 0});

        // Turn open and write failures into exceptions.
        stream_.exceptions(std::ios::badbit | std::ios::failbit);
        stream_.open(directory_ / files_.back().path);
    }

    // Preserve archived header precision and numbering. Uniform IDs
    // restart from zero in each split file; physical IDs retain the source entry index stored in e.id.
    const auto id = static_cast<unsigned long long>(workflow_ == "uniform" ? files_.back().events : e.id);

    // electron–hadron samples and the beam-momentum electron tester historically wrote beam energy with one decimal;
    // ordinary 1e and physical conversion used six decimals. Other header fields retain their
    // archived precision and meanings, including physical process tags in e.weight.
    const bool electron_hadron = workflow_ == "uniform" && config_.get("channel") == "eh";
    const bool tester = workflow_ == "uniform" && config_.get("channel") == "electron-tester";
    const char* format =
        electron_hadron || tester ? "%i \t %i \t %i \t %.3f \t %.3f \t %i \t %.1f \t %i \t %llu \t %.3f \n" : "%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %llu \t %.2f \n";
    stream_ << TString::Format(format, static_cast<int>(e.particles.size()), e.A, e.Z, e.resonance_id, 0., constants::electron_pdg, e.beam_energy, 1, id, e.weight);

    // Calculate E=sqrt(m²+p²) with c=1 and write particles in their stored order.
    int index = 0;
    for (const auto& p : e.particles) {
        const double energy = std::sqrt(p.mass * p.mass + p.momentum.Mag2());

        // Reject invalid energy or vertex values before writing the particle.
        if (!std::isfinite(energy) || !std::isfinite(p.vertex.Mag2())) { throw std::runtime_error("Non-finite particle data"); }
        // Particle records retain tabs, status/parent zeros, active flag 1, and five decimal places.
        stream_ << TString::Format("%i \t %.3f \t %i \t %i \t %i \t %i \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \n", ++index, 0., 1, p.pid, 0, 0, p.momentum.X(),
                                   p.momentum.Y(), p.momentum.Z(), energy, p.mass, p.vertex.X(), p.vertex.Y(), p.vertex.Z());
    }

    // Increase the counts only after the complete event is written.
    ++files_.back().events;
    ++count_;
}
#pragma endregion

// LundWriter::finish ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::finish */
void LundWriter::finish(std::uint64_t scanned) {
    // Close the LUND file before recording its final counts.
    if (stream_.is_open()) { stream_.close(); }

    // Write a temporary log so readers never see a partly written final file.
    std::ofstream manifest;
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    const auto monitoring_directory = directory_ / "lundfiles" / "lund-gen-monitoring";
    const auto temporary_log = monitoring_directory / "lund-gen-log.json.tmp";
    const auto completed_log = monitoring_directory / "lund-gen-log.json";
    manifest.open(temporary_log);

    // Record the log version, project build, Git state, ROOT version, targets.h hash, and event
    // counts. scanned may be larger than count_ when physical input events are rejected.
    manifest << "{\n  \"schema_version\": 1,\n  \"workflow\": " << jsonString(workflow_) << ",\n  \"version\": " << jsonString(SAMPLE_VERSION)
             << ",\n  \"revision\": " << jsonString(SAMPLE_REVISION) << ",\n  \"root_version\": " << jsonString(gROOT->GetVersion())
             << ",\n  \"targets_sha256\": " << jsonString(SAMPLE_TARGETS_SHA256) << ",\n  \"git\": {\n"
             << "    \"repository\": " << jsonString(SAMPLE_GIT_REPOSITORY) << ",\n"
             << "    \"branch\": " << jsonString(SAMPLE_GIT_BRANCH) << ",\n"
             << "    \"commit_message\": " << jsonString(SAMPLE_GIT_COMMIT_MESSAGE) << ",\n"
             << "    \"full_commit_hash\": " << jsonString(SAMPLE_GIT_COMMIT_HASH) << ",\n"
             << "    \"commit_datetime\": " << jsonString(SAMPLE_GIT_COMMIT_DATETIME) << ",\n"
             << "    \"commit_author\": " << jsonString(SAMPLE_GIT_COMMIT_AUTHOR) << ",\n"
             << "    \"status_porcelain_summary\": " << jsonString(SAMPLE_GIT_STATUS) << ",\n"
             << "    \"nearest_tag\": " << jsonString(SAMPLE_GIT_NEAREST_TAG) << ",\n"
             << "    \"head_detached\": " << SAMPLE_GIT_HEAD_DETACHED << ",\n"
             << "    \"tracking_branch\": " << jsonString(SAMPLE_GIT_TRACKING_BRANCH) << ",\n"
             << "    \"tracking_ahead\": " << jsonString(SAMPLE_GIT_TRACKING_AHEAD) << ",\n"
             << "    \"tracking_behind\": " << jsonString(SAMPLE_GIT_TRACKING_BEHIND) << ",\n"
             << "    \"github_files_url\": " << jsonString(SAMPLE_GIT_FILES_URL) << "\n  },\n"
             << "  \"scanned_events\": " << scanned << ",\n  \"written_events\": " << count_ << ",\n  \"config\": {";

    // std::map keeps keys sorted; jsonString() writes safe JSON text.
    bool first = true;
    for (const auto& [k, v] : config_.values()) {
        manifest << (first ? "\n" : ",\n") << "    " << jsonString(k) << ": " << jsonString(v);
        first = false;
    }

    // File records remain in creation order and use paths relative to the run directory. Their event
    // counts include a partial final file and sum to written_events for a successfully completed run.
    manifest << "\n  },\n  \"files\": [";
    first = true;
    for (const auto& file : files_) {
        manifest << (first ? "\n" : ",\n") << "    {\"path\": " << jsonString(file.path) << ", \"events\": " << file.events << '}';
        first = false;
    }
    manifest << "\n  ]\n}\n";

    // Close the temporary file before renaming it. The final name marks the run as complete.
    manifest.close();
    std::filesystem::rename(temporary_log, completed_log);
}
#pragma endregion

}  // namespace samples
