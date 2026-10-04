/**
 * @file LundWriter.cpp
 * @brief Creates the run directory, writes numbered LUND files, and publishes the completed-run manifest.
 *
 * Purpose:
 *   Give the uniform LUND creator and physical LUND converter the same output process. This file prints
 *   the resolved settings, safely replaces the exact run directory, writes events in the LUND text format,
 *   creates empty directories for later simulation output, and records what the completed run produced.
 *
 * Execution flow:
 *   printWorkflowSummary() shows the setup. The constructor checks and replaces the exact run directory.
 *   writeEvent() opens file 1 when the first event arrives, opens later numbered files as each file fills,
 *   and writes every event header followed by its particles. After any required monitoring output is
 *   saved, finalizeRun() closes the active file, writes `lund-creation-log.json.tmp`, and renames it to
 *   `lund-creation-log.json` to mark the run complete.
 *
 * Written format:
 *   LUND fields use fixed spaces, tabs, and numbers of decimal places. The uniform LUND creator supplies
 *   event IDs that begin at zero and continue across files. The physical LUND converter supplies the input
 *   GST entry number. Particle records use momentum in GeV/c, mass in GeV/c², calculated energy in GeV,
 *   and vertex positions in centimeters.
 *
 * Failure:
 *   A path that could delete the filesystem root, home directory, current directory, or source checkout is
 *   rejected before anything is removed. A file or directory failure reports an error and may leave
 *   partial output for inspection. Without `lund-creation-log.json`, that output is not a completed run.
 */

#include "core/lund/LundWriter.h"

#include <TROOT.h>
#include <TString.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "Version.h"

namespace env = environment;

namespace samples {

// Preparing the run directory -------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Preparing the run directory */
LundWriter::LundWriter(const RunConfig& c, std::string workflow)
    : config_(c),
      workflow_(std::move(workflow)),
      directory_(c.getText("output")),
      events_per_file_(c.getNonnegativeInteger("events-per-file")),
      capacity_(c.getNonnegativeInteger("events")) {
    // Make the output path absolute and remove `.` and `..` parts immediately before this constructor may
    // delete an existing directory.
    directory_ = std::filesystem::absolute(directory_).lexically_normal();

    // Collect locations that must never be deleted: the filesystem root, source checkout, home directory,
    // and current directory. HOME may be unavailable in some batch jobs.
    const auto root = directory_.root_path();
    const auto source = std::filesystem::path(SAMPLE_SOURCE_DIR).lexically_normal();
    const char* home_value = std::getenv("HOME");
    const auto home = home_value ? std::filesystem::path(home_value).lexically_normal() : std::filesystem::path();

    // The added separator makes a text-prefix check distinguish a true child path such as
    // `/work/run/source` from an unrelated path such as `/work/run-old`.
    const auto directory_text = directory_.string() + std::filesystem::path::preferred_separator;
    const bool contains_checkout = ((source == directory_) || (source.string().rfind(directory_text, 0) == 0));

    // Stop before deletion if the run path is empty, too broad, or contains the source checkout.
    if (directory_.empty() || (directory_ == root) || (!home.empty() && (directory_ == home)) || directory_.filename().empty() || (directory_ == std::filesystem::current_path()) ||
        contains_checkout) {
        throw std::runtime_error("Refusing unsafe output-directory replacement: " + directory_.string());
    }

    // Create missing parent directories. If the exact run directory already exists, warn and replace only
    // that checked path.
    std::filesystem::create_directories(directory_.parent_path());
    if (std::filesystem::exists(directory_)) {
        std::cout << env::WARNING_COLOR << "Warning:" << env::RESET_COLOR << " Replacing existing run directory: " << directory_ << "\n";
        std::filesystem::remove_all(directory_);
    }

    // Both LUND workflows create the same empty mchipo and reconhipo directories for later GEMC simulation
    // and CLAS12 reconstruction output.
    std::filesystem::create_directories(directory_ / "lundfiles" / "lund-creation-monitoring");
    for (const auto* child : {"mchipo", "reconhipo"}) { std::filesystem::create_directories(directory_ / child); }

    // writeEvent() opens the first LUND file only when an event is ready, which avoids an empty numbered file.
    // Only the uniform LUND creator produces rendered monitoring plots.
    if (workflow_ == "uniform") { std::filesystem::create_directories(directory_ / "lundfiles" / "lund-creation-monitoring" / "MonitoringPlotsPath"); }
}
#pragma endregion

// Writing one event -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Writing one event */
void LundWriter::writeEvent(const Event& e) {
    // Check the event limit here even if the caller already called hasReachedRunEventLimit(). LUND cannot
    // represent an event with no particles in this workflow.
    if (hasReachedRunEventLimit()) { throw std::runtime_error("Run file limit reached"); }
    if (e.particles.empty()) { throw std::runtime_error("Cannot write an empty event"); }

    // Open a file only when an event is ready for it. This prevents an empty file at the end of a run.
    if (files_.empty() || (files_.back().events == events_per_file_)) {
        // Close the previous file first so all of its buffered text reaches storage before the next file
        // opens.
        if (stream_.is_open()) { stream_.close(); }

        // Number files starting at 1. Store each path below the run directory so the complete run folder
        // can be moved without making its manifest paths incorrect.
        files_.push_back({"lundfiles/" + config_.getText("prefix") + "_" + std::to_string(files_.size() + 1) + ".txt", 0});

        // Ask the output stream to report file-opening and writing problems as exceptions.
        stream_.exceptions(std::ios::badbit | std::ios::failbit);
        stream_.open(directory_ / files_.back().path);
    }

    // The shared LUND header format writes the event ID with `%d`, so require the value to fit its matching
    // int argument before conversion. Uniform IDs begin at zero across all files. Physical IDs keep the GST
    // input-entry number.
    if (e.id > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) { throw std::runtime_error("Event ID exceeds the LUND header integer range"); }
    const auto id = static_cast<int>(e.id);

    // Use this one exact header format for every event. For physical events, e.resonance_id contains GST
    // `resid` under the documented RG-M compatibility convention; uniform events supply zero. Field 8 is
    // the fixed value 1 widely used by RG-M LUND writers, not an interaction count. `%f` writes six digits
    // after the decimal point, so a beam energy such as 5.98636 becomes 5.986360 instead of 6.0. The last
    // field uses two digits after the decimal point and stores the physical interaction code in e.weight.
    stream_ << TString::Format("%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %d \t %.2f \n", static_cast<int>(e.particles.size()), e.A, e.Z, e.resonance_id, 0., constants::electron_pdg,
                               e.beam_energy, 1, id, e.weight);

    // In the units used by LUND, the speed of light is treated as 1. This gives
    // energy = sqrt(mass² + momentum²). Write particles in their stored order.
    int index = 0;
    for (const auto& p : e.particles) {
        const double energy = std::sqrt(p.mass * p.mass + p.momentum.Mag2());

        // Stop before writing this particle if its calculated energy or any vertex-position coordinate is
        // infinite or is not a number.
        if (!std::isfinite(energy) || !std::isfinite(p.vertex.Mag2())) { throw std::runtime_error("Non-finite particle data"); }
        // Separate fields with tabs. Write zero for the unused status and parent fields, 1 to mark the
        // particle active, and five digits after the decimal point for momentum, energy, mass, and position.
        stream_ << TString::Format("%i \t %.3f \t %i \t %i \t %i \t %i \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \n", ++index, 0., 1, p.pid, 0, 0, p.momentum.X(),
                                   p.momentum.Y(), p.momentum.Z(), energy, p.mass, p.vertex.X(), p.vertex.Y(), p.vertex.Z());
    }

    // Increase both counts only after the event header and every particle record have been written.
    ++files_.back().events;
    ++count_;
}
#pragma endregion

// Publishing the completed-run manifest ---------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Publishing the completed-run manifest */
void LundWriter::finalizeRun(std::uint64_t scannedEventCount) {
    // Close the active LUND file before recording the final counts and file list.
    if (stream_.is_open()) { stream_.close(); }

    // Write to a temporary name first. Until the complete JSON document is closed and renamed, readers do
    // not see the final filename that marks a successful run.
    std::ofstream manifest;
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    const auto monitoring_directory = directory_ / "lundfiles" / "lund-creation-monitoring";
    const auto temporary_log = monitoring_directory / "lund-creation-log.json.tmp";
    const auto completed_log = monitoring_directory / "lund-creation-log.json";
    manifest.open(temporary_log);

    // Record the manifest format version, application build, Git details, ROOT version, targets.h fingerprint,
    // and event counts. For the physical LUND converter, scannedEventCount may be larger than count_ because
    // unsupported input events are skipped.
    manifest << "{\n  \"schema_version\": 1,\n  \"workflow\": " << quoteAsJsonString(workflow_) << ",\n  \"version\": " << quoteAsJsonString(SAMPLE_VERSION)
             << ",\n  \"revision\": " << quoteAsJsonString(SAMPLE_REVISION) << ",\n  \"root_version\": " << quoteAsJsonString(gROOT->GetVersion())
             << ",\n  \"targets_sha256\": " << quoteAsJsonString(SAMPLE_TARGETS_SHA256) << ",\n  \"git\": {\n"
             << "    \"repository\": " << quoteAsJsonString(SAMPLE_GIT_REPOSITORY) << ",\n"
             << "    \"branch\": " << quoteAsJsonString(SAMPLE_GIT_BRANCH) << ",\n"
             << "    \"commit_message\": " << quoteAsJsonString(SAMPLE_GIT_COMMIT_MESSAGE) << ",\n"
             << "    \"full_commit_hash\": " << quoteAsJsonString(SAMPLE_GIT_COMMIT_HASH) << ",\n"
             << "    \"commit_datetime\": " << quoteAsJsonString(SAMPLE_GIT_COMMIT_DATETIME) << ",\n"
             << "    \"commit_author\": " << quoteAsJsonString(SAMPLE_GIT_COMMIT_AUTHOR) << ",\n"
             << "    \"status_porcelain_summary\": " << quoteAsJsonString(SAMPLE_GIT_STATUS) << ",\n"
             << "    \"nearest_tag\": " << quoteAsJsonString(SAMPLE_GIT_NEAREST_TAG) << ",\n"
             << "    \"head_detached\": " << SAMPLE_GIT_HEAD_DETACHED << ",\n"
             << "    \"tracking_branch\": " << quoteAsJsonString(SAMPLE_GIT_TRACKING_BRANCH) << ",\n"
             << "    \"tracking_ahead\": " << quoteAsJsonString(SAMPLE_GIT_TRACKING_AHEAD) << ",\n"
             << "    \"tracking_behind\": " << quoteAsJsonString(SAMPLE_GIT_TRACKING_BEHIND) << ",\n"
             << "    \"github_files_url\": " << quoteAsJsonString(SAMPLE_GIT_FILES_URL) << "\n  },\n"
             << "  \"scanned_events\": " << scannedEventCount << ",\n  \"written_events\": " << count_ << ",\n  \"config\": {";

    // RunConfig stores settings in a map, which visits names in sorted order. quoteAsJsonString() adds the
    // quotes and special-character escaping required by JSON.
    bool first = true;
    for (const auto& [k, v] : config_.getAllSettings()) {
        manifest << (first ? "\n" : ",\n") << "    " << quoteAsJsonString(k) << ": " << quoteAsJsonString(v);
        first = false;
    }

    // List files in creation order using paths below the run directory. The last file may contain fewer
    // events than the per-file limit. In a completed run, adding these counts gives written_events.
    manifest << "\n  },\n  \"files\": [";
    first = true;
    for (const auto& file : files_) {
        manifest << (first ? "\n" : ",\n") << "    {\"path\": " << quoteAsJsonString(file.path) << ", \"events\": " << file.events << '}';
        first = false;
    }
    manifest << "\n  ]\n}\n";

    // Close the temporary file so all JSON text reaches storage, then rename it. The final name is the
    // completion marker checked by later workflow stages.
    manifest.close();
    std::filesystem::rename(temporary_log, completed_log);
}
#pragma endregion

// Printing setup and completion summaries -------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Printing setup and completion summaries */
void LundWriter::printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned, std::uint64_t written, bool final) {
    // The `uniform` value selects settings and monitoring paths used only by the uniform LUND creator.
    // Constructing these path values does not create any file or directory.
    const bool uniform = (workflow == "uniform");
    const auto output = std::filesystem::path(config.getText("output"));
    const auto lund_dir = output / "lundfiles";
    const auto diagnostics = output / "lundfiles" / "lund-creation-monitoring";
    const auto summary_title = (uniform ? "Uniform LUND creator" : "Physical LUND converter") + std::string(final ? " completion" : " setup");

    // Align normal values near the right side of the 100-character summary. Print paths immediately after
    // their labels because added spaces or quotes would make copied paths harder to use.
    const auto print_value = [](const std::string& label, const auto& value) {
        std::ostringstream rendered_value;
        rendered_value << value;

        const auto text = rendered_value.str();
        const auto label_width = label.size() + 1;
        const auto padding = (label_width + text.size() < 99) ? 99 - label_width - text.size() : 1;

        std::cout << env::SYSTEM_COLOR << label << ":" << env::RESET_COLOR << std::string(padding, ' ') << text << "\n";
    };
    const auto print_path = [](const std::string& label, const std::filesystem::path& path) {
        std::cout << env::SYSTEM_COLOR << label << ":" << env::RESET_COLOR << " " << path.string() << "\n";
    };

    // Surround the title with visible lines so the summary is easy to find in a long batch-job log.
    std::cout << env::SYSTEM_COLOR << "\n====================================================================================================\n"
              << "= " << summary_title << std::string(96 - summary_title.size(), ' ') << " =\n"
              << "====================================================================================================\n"
              << env::RESET_COLOR;

    // The final report shows only event and file counts; it does not repeat the earlier setup.
    if (final) {
        const auto events_per_file = config.getNonnegativeInteger("events-per-file");
        const auto output_files = (written + events_per_file - 1) / events_per_file;

        std::cout << env::SYSTEM_COLOR << "\n- Event counts -------------------------------------------------------------------------------------\n" << env::RESET_COLOR;
        print_value(uniform ? "Events generated" : "Input entries scanned", scanned);
        print_value("Events written", written);
        print_value("Events per file", config.getText("events-per-file"));
        print_value("LUND files written", output_files);
        std::cout << "\n";

        return;
    }

    // These two limits control the complete run and when the writer starts a new numbered file.
    std::cout << env::SYSTEM_COLOR << "\n- Run limits ---------------------------------------------------------------------------------------\n" << env::RESET_COLOR;
    print_value("Requested events", config.getText("events"));
    print_value("Events per file", config.getText("events-per-file"));

    // Beam energy, A, and Z are written in event headers. The target geometry separately controls where an
    // event occurs. A is the total number of protons and neutrons; Z is the number of protons.
    std::cout << env::SYSTEM_COLOR << "\n- Beam and target ----------------------------------------------------------------------------------\n" << env::RESET_COLOR;
    print_value("Beam energy [GeV]", config.getText("beam-energy"));
    print_value("Target", config.getText("target"));
    print_value("GEMC target variation", config.getText("gemc-target-variation"));
    print_value("Target geometry", config.getText("target-geometry"));
    print_value("Target A", config.getText("A"));
    print_value("Target Z", config.getText("Z"));
    print_value("Vertex-position seed", config.getText("vertex-seed") + ((config.getText("vertex-seed") == "0") ? " (ROOT automatic; nonrepeatable)" : ""));

    if (uniform) {
        const auto channel = config.getText("channel");
        const bool electron_hadron = (channel == "eh");
        const bool electron_tester = (channel == "electron-tester");

        // Show only the particle-motion settings used by the selected uniform channel.
        std::cout << env::SYSTEM_COLOR << "\n- Uniform event content ----------------------------------------------------------------------------\n" << env::RESET_COLOR;
        print_value("Channel", channel);
        print_value("Kinematic seed", config.getText("seed") + ((config.getText("seed") == "0") ? " (ROOT automatic; nonrepeatable)" : ""));

        if (electron_hadron) {
            print_value("Hadron", config.getText("hadron"));
            print_value("Hadron region", config.getText("hadron-region"));
            print_value("Hadron momentum mode", config.getText("hadron-momentum"));
            print_value("Hadron theta minimum [deg]", config.getText("hadron-theta-min"));
            print_value("Hadron theta maximum [deg]", config.getText("hadron-theta-max"));

            if (config.getText("hadron-momentum") == "fixed") {
                print_value("Hadron momentum [GeV/c]", config.getText("hadron-p"));
            } else {
                print_value("Hadron momentum minimum [GeV/c]", config.getText("hadron-p-min"));
                print_value("Hadron momentum maximum [GeV/c]", config.getText("beam-energy"));
            }

            print_value("Trigger-electron theta [deg]", config.getText("trigger-theta"));
            print_value("Trigger-electron phi offset [deg]", config.getText("trigger-phi-offset"));
        } else {
            print_value("Electron momentum mode", config.getText("electron-momentum"));
            print_value("Electron theta minimum [deg]", config.getText("electron-theta-min"));
            print_value("Electron theta maximum [deg]", config.getText("electron-theta-max"));

            if (!electron_tester) {
                print_value("Electron momentum minimum [GeV/c]", config.getText("electron-p-min"));
                print_value("Electron momentum maximum [GeV/c]", config.getText("electron-p-max"));
            }
        }
    } else {
        // These values identify the physical input in output names and the completed-run manifest.
        std::cout << env::SYSTEM_COLOR << "\n- Physical input -----------------------------------------------------------------------------------\n" << env::RESET_COLOR;
        print_value("Event generator", config.getText("event-generator"));
        print_value("Event generator version", config.getText("event-generator-version"));
        print_value("Input files", config.getText("input"));
        print_value("Generator tune", config.getText("tune"));
        print_value("Q2-cut label", config.getText("q2-cut"));
    }

    // Show only paths that the selected LUND workflow creates or later uses.
    std::cout << env::SYSTEM_COLOR << "\n- Output -------------------------------------------------------------------------------------------\n" << env::RESET_COLOR;
    print_value("Output prefix", config.getText("prefix"));
    print_path("Run directory", output);
    print_path("LUND directory", lund_dir);
    print_path("Completion manifest", diagnostics / "lund-creation-log.json");

    print_path("MC HIPO directory", output / "mchipo");
    print_path("Reconstructed HIPO directory", output / "reconhipo");

    if (uniform) {
        print_path("Monitoring ROOT file", diagnostics / (config.getText("prefix") + "__monitoring_plots.root"));
        print_path("Monitoring plot directory", diagnostics / "MonitoringPlotsPath");
    }

    std::cout << "\n";
}
#pragma endregion

}  // namespace samples
