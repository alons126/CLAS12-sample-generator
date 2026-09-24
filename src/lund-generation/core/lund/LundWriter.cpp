//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.cpp
 * @brief LUND serialization and completed-run manifests.
 *
 * Purpose:
 *   Implement the common output boundary for uniform and physical LUND creation: topic-grouped run
 *   reporting, guarded directory replacement, split-file serialization, exact bookkeeping, resolved-
 *   configuration provenance, and completion publication.
 *
 * Workflow:
 *   Print resolved setup -> validate and claim the exact run directory -> lazily open/rotate LUND files
 *   -> serialize headers and ordered particles -> let the source save diagnostics -> close the active
 *   stream -> write lundfiles/lund-gen-monitoring/lund-gen-log.json.tmp -> atomically rename it to
 *   lund-gen-log.json.
 *
 * Format contract:
 *   The LUND format uses fixed whitespace, precision, and uniform per-file IDs. Particle masses come
 *   from the external target source through particleMass(). Records use momentum in GeV/c, mass in
 *   GeV/c², energy in GeV, and vertices in cm.
 *
 * Failure behavior:
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
#include "core/support/environment.h"

namespace env = environment;

namespace samples {

// LundWriter::printWorkflowSummary --------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::printWorkflowSummary */
void LundWriter::printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned, std::uint64_t written, bool final) {
    // Maintained callers use exactly `uniform` or `physical`; only the former selects uniform settings
    // and paths. Constructing the display-only paths below has no filesystem side effects.
    const bool uniform = workflow == "uniform";
    const auto output = std::filesystem::path(config.get("output"));
    const auto lund_dir = output / "lundfiles";
    const auto diagnostics = output / "lundfiles" / "lund-gen-monitoring";

    // Centralize the one-value-per-line presentation contract. The label includes its separator, while
    // value preserves the stream representation of strings, numbers and filesystem paths.
    const auto print_value = [](const std::string& label, const auto& value) { std::cout << env::SYSTEM_COLOR << label << ":" << env::RESET_COLOR << " " << value << "\n"; };

    // Keep a prominent colored boundary so setup and completion remain visible in long batch logs.
    std::cout << env::SYSTEM_COLOR << "\n=============================================================\n"
              << "= " << (uniform ? "Uniform sample generation" : "Physical generator to LUND conversion") << (final ? " completion\n" : " setup\n")
              << "=============================================================\n"
              << env::RESET_COLOR;

    // Completion reports deliberately do not repeat setup values already printed before output creation.
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

    // Run limits apply to both sources and directly control writer capacity and file rotation.
    std::cout << env::SYSTEM_COLOR << "\n- Run limits ------------------------------------------------\n" << env::RESET_COLOR;
    print_value("Requested events", config.get("events"));
    print_value("Events per file", config.get("events-per-file"));

    // Beam and target settings control event headers and vertex sampling. RG-M identity also supplies
    // resolved defaults, while geometry and A/Z retain their distinct spatial/metadata responsibilities.
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

        // Print only kinematic settings consumed by the selected uniform branch. Values configured for a
        // different branch are intentionally absent even though RunConfig retains them for validation.
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
        // Physical settings identify the input adapter and the provenance used by naming and manifests.
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
    // Re-normalize defensively at the destructive-operation boundary even though RunConfig::parse()
    // already returns an absolute output path.
    directory_ = std::filesystem::absolute(directory_).lexically_normal();

    // Resolve high-risk reference paths without requiring HOME to exist. SAMPLE_SOURCE_DIR is supplied
    // by the build and identifies the checkout whose deletion (or deletion through an ancestor) must be
    // refused.
    const auto root = directory_.root_path();
    const auto source = std::filesystem::path(SAMPLE_SOURCE_DIR).lexically_normal();
    const char* home_value = std::getenv("HOME");
    const auto home = home_value ? std::filesystem::path(home_value).lexically_normal() : std::filesystem::path();

    // Appending the preferred separator makes the prefix test path-component-aware: `/work/run` is an
    // ancestor of `/work/run/source`, while `/work/run-old` is not.
    const auto directory_text = directory_.string() + std::filesystem::path::preferred_separator;
    const bool contains_checkout = source == directory_ || source.string().rfind(directory_text, 0) == 0;

    // Refuse paths whose recursive removal could erase a filesystem root, user home, active working
    // directory, the source checkout, or an ancestor containing that checkout.
    if (directory_.empty() || directory_ == root || (!home.empty() && directory_ == home) || directory_.filename().empty() || directory_ == std::filesystem::current_path() ||
        contains_checkout) {
        throw std::runtime_error("Refusing unsafe output-directory replacement: " + directory_.string());
    }

    // Ensure the parent exists before checking/replacing the final run. Existing contents at the exact
    // final path are disposable by the documented legacy rerun contract.
    std::filesystem::create_directories(directory_.parent_path());
    if (std::filesystem::exists(directory_)) {
        std::cout << env::WARNING_COLOR << "Replacing existing run directory (legacy behavior): " << directory_ << env::RESET_COLOR << "\n";
        std::filesystem::remove_all(directory_);
    }

    // LUND streams are opened lazily by write(); no empty numbered file is created at construction.
    std::filesystem::create_directories(directory_ / "lundfiles" / "lund-gen-monitoring");

    // Uniform creation historically prepared the complete downstream directory layout and plot folder
    // before event generation. Keep the directories consumed by later simulation and reconstruction
    // workflows without running either workflow here.
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
    // Enforce capacity internally even when a caller forgets to check full(). LUND events must contain
    // at least one particle because the header multiplicity and following records form one unit.
    if (full()) { throw std::runtime_error("Run file limit reached"); }
    if (e.particles.empty()) { throw std::runtime_error("Cannot write an empty event"); }

    // Open a new file only when the next event actually needs one. This avoids empty trailing files for
    // exact multiples of the configured split size and allows a partially filled final file.
    if (files_.empty() || files_.back().events == events_per_file_) {
        // Closing the previous stream flushes it before a new manifest record and path are selected.
        if (stream_.is_open()) { stream_.close(); }

        // Number files from one in creation order and store a run-relative path for portable manifests.
        files_.push_back({"lundfiles/" + config_.get("prefix") + "_" + std::to_string(files_.size() + 1) + ".txt", 0});

        // Convert bad/fail stream states into exceptions. Records below use explicit archived field precision.
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

    // Derive mass-shell energy E=sqrt(m²+p²) under c=1 and emit fourteen fields per particle. The loop
    // follows Event::particles order exactly; no source-specific sorting or filtering occurs here.
    int index = 0;
    for (const auto& p : e.particles) {
        const double energy = std::sqrt(p.mass * p.mass + p.momentum.Mag2());

        // A non-finite momentum or mass propagates into energy; checking vertex magnitude catches any
        // non-finite coordinate. Reject before emitting that particle record.
        if (!std::isfinite(energy) || !std::isfinite(p.vertex.Mag2())) { throw std::runtime_error("Non-finite particle data"); }
        // Particle records retain tabs, status/parent zeros, active flag 1, and five decimal places.
        stream_ << TString::Format("%i \t %.3f \t %i \t %i \t %i \t %i \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \n", ++index, 0., 1, p.pid, 0, 0, p.momentum.X(),
                                   p.momentum.Y(), p.momentum.Z(), energy, p.mass, p.vertex.X(), p.vertex.Y(), p.vertex.Z());
    }

    // Commit bookkeeping only after the full event has reached the stream successfully.
    ++files_.back().events;
    ++count_;
}
#pragma endregion

// LundWriter::finish ----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* LundWriter::finish */
void LundWriter::finish(std::uint64_t scanned) {
    // Close first so every numbered LUND file is flushed and no further event can be appended through
    // the active stream while its counts are being published.
    if (stream_.is_open()) { stream_.close(); }

    // Write completion metadata to a temporary sibling so consumers never observe a partially written
    // final manifest. Stream exceptions turn open, serialization, flush, and close failures into the
    // same workflow error path used by LUND output.
    std::ofstream manifest;
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    const auto monitoring_directory = directory_ / "lundfiles" / "lund-gen-monitoring";
    const auto temporary_log = monitoring_directory / "lund-gen-log.json.tmp";
    const auto completed_log = monitoring_directory / "lund-gen-log.json";
    manifest.open(temporary_log);

    // Fixed top-level provenance identifies the manifest schema, this project build/revision, the ROOT
    // runtime, and the exact external targets.h content compiled into the application. count_ records
    // successful serialization independently of how many source events were examined.
    manifest << "{\n  \"schema_version\": 1,\n  \"workflow\": " << jsonString(workflow_) << ",\n  \"version\": " << jsonString(SAMPLE_VERSION)
             << ",\n  \"revision\": " << jsonString(SAMPLE_REVISION) << ",\n  \"root_version\": " << jsonString(gROOT->GetVersion())
             << ",\n  \"targets_sha256\": " << jsonString(SAMPLE_TARGETS_SHA256) << ",\n  \"scanned_events\": " << scanned << ",\n  \"written_events\": " << count_ << ",\n  \"config\": {";

    // RunConfig stores a std::map, so iteration produces deterministic key order. jsonString() escapes
    // both names and resolved values while preserving the exact provenance spelling used by the run.
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

    // Explicit close verifies buffered manifest output before publication. The sibling rename is the
    // visibility boundary: only after it succeeds does lund-gen-log.json advertise a completed run.
    manifest.close();
    std::filesystem::rename(temporary_log, completed_log);
}
#pragma endregion

}  // namespace samples
