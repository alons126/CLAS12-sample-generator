//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.cpp
 * @brief LUND serialization and completed-run manifests.
 *
 * Purpose:
 *   Implement the common output boundary for uniform and physical LUND creation: legacy-compatible
 *   monitoring text, guarded run-directory replacement, split-file serialization, exact bookkeeping,
 *   resolved-configuration provenance, and completion publication.
 *
 * Workflow:
 *   Print resolved setup -> validate and claim the exact run directory -> lazily open/rotate LUND files
 *   -> serialize headers and ordered particles -> let the source save diagnostics -> close the active
 *   stream -> write manifest.json.tmp -> atomically rename it to manifest.json.
 *
 * Compatibility:
 *   Legacy LUND format preserves archived whitespace, precision, and uniform per-file IDs. Precise
 *   format uses the same event content with higher numeric precision and run-global IDs; the separate
 *   mass-convention setting selects particle masses. Both formats use momentum in GeV/c, mass in
 *   GeV/c², energy in GeV, and vertices in cm.
 *
 * Failure behavior:
 *   Unsafe replacement targets are rejected before deletion. Stream and filesystem failures throw and
 *   may leave partial output for inspection; absence of manifest.json marks the run incomplete.
 */

#include "common/LundWriter.h"

#include <TROOT.h>
#include <TString.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "Version.h"
#include "common/environment.h"

namespace samples {

// LundWriter::printWorkflowSummary ----------------------------------------------------------------------

#pragma region /* LundWriter::printWorkflowSummary */
/**
 * @brief Print the legacy-style setup and completion summary for a LUND run.
 *
 * Purpose:
 *   Preserve the recognizable monitoring printouts from both archived repositories while adding the
 *   shared resolved settings needed to diagnose a maintained run from terminal or batch logs.
 *
 * Workflow:
 *   1. Derive display-only subdirectory paths from the resolved run directory.
 *   2. Print the uniform CodeRun-style block or physical converter-style input block.
 *   3. Print shared output, beam, target, capacity, and serialization settings.
 *   4. For a final report, print scanned/written counters and derive the split-file count.
 *
 * @param config Borrowed resolved workflow configuration used only for display.
 * @param workflow Source label. `uniform` selects uniform text; the maintained caller passes `physical`
 *                 for conversion text.
 * @param scanned Number of source entries examined. Uniform generation supplies its generated/written
 *                count; physical conversion also includes rejected interaction types.
 * @param written Number of events successfully serialized into LUND output.
 * @param final Print completion counters when true; print setup fields only when false.
 *
 * @return Nothing. Text is written to standard output with ANSI color sequences.
 *
 * @note Presentation only: derived mchipo/reconhipo/rootfiles/monitoring paths reproduce legacy status
 *       text but are not created here. LundWriter construction creates only the run and `lundfiles/`
 *       directories; later simulation and monitoring stages own their outputs.
 *
 * @note The historical uniform `nParticles: 2` line is retained as legacy monitoring text even though
 *       the 1e channel writes one particle. Event serialization always uses Event::particles.size().
 */
void LundWriter::printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned, std::uint64_t written, bool final) {
    // These are display paths below the already resolved final run directory. Constructing path values
    // has no filesystem side effects.
    const auto output = std::filesystem::path(config.get("output"));
    const auto lund_dir = output / "lundfiles";
    const auto mchipo_dir = output / "mchipo";
    const auto recon_dir = output / "reconhipo";
    const auto rootfiles_dir = output / "rootfiles";
    const auto monitoring_dir = output / "monitoring_plots";
    // Maintained callers use exactly `uniform` or `physical`; only the former selects the uniform block.
    const bool uniform = workflow == "uniform";

    // Keep the archived yellow separator style so long interactive and Slurm logs expose run boundaries.
    std::cout << "\033[33m\n=============================================================\n\033[0m";
    std::cout << "\033[33m\n= " << (uniform ? "Uniform sample generation" : "Physical generator to LUND conversion") << " summary" << "\n\033[0m";
    std::cout << "\033[33m=============================================================\n\033[0m";

    if (uniform) {
        // Reproduce CodeRun-style labels and constants alongside the resolved channel/mode. Several
        // listed downstream paths are planning information for later GEMC/reconstruction workflows.
        std::cout << "\033[33m\nOutputFileNamePrefix:\033[0m " << config.get("prefix") << '\n';
        std::cout << "\033[33mRequested events:\033[0m " << config.get("events") << "  \033[33mEvents per file:\033[0m 10000\n";
        std::cout << "\033[33mBeam energy [GeV]:\033[0m " << config.get("beam-energy") << '\n';
        std::cout << "\033[33mGenerateLundFiles:\033[0m true\n";
        std::cout << "\033[33mnParticles:\033[0m 2\n";
        std::cout << "\033[33mmass_e [GeV/c²]:\033[0m " << 0.511e-3 << "  \033[33mmass_p [GeV/c²]:\033[0m " << 0.938272 << "  \033[33mmass_n [GeV/c²]:\033[0m " << 0.93957 << '\n';
        std::cout << "\033[33mOutPutFolder:\033[0m " << output << '\n';
        std::cout << "\033[33mlundPath:\033[0m " << lund_dir << '\n';
        std::cout << "\033[33mmchipoPath:\033[0m " << mchipo_dir << '\n';
        std::cout << "\033[33mreconhipoPath:\033[0m " << recon_dir << '\n';
        std::cout << "\033[33mrootfilesPath:\033[0m " << rootfiles_dir << '\n';
        std::cout << "\033[33mMonitoringPlotsPath:\033[0m " << monitoring_dir << '\n';
        std::cout << "\033[33mPlot list path:\033[0m " << output / (config.get("prefix") + "_plots.root") << '\n';
        std::cout << "\033[33mChannel:\033[0m " << config.get("channel") << "  \033[33mElectron momentum:\033[0m " << config.get("electron-momentum") << "  \033[33mNucleon momentum:\033[0m "
                  << config.get("nucleon-momentum") << '\n';
        std::cout << "\033[33mtargP:\033[0m 0  \033[33mbeamP:\033[0m 0  \033[33minteractN:\033[0m 1  \033[33mbeamType:\033[0m 11\n";
        std::cout << "\033[33mbeamE_in_lundfiles:\033[0m " << config.get("beam-energy") << "\n";
        std::cout << "\033[33mweight:\033[0m 1\n";
        std::cout << "\033[33mCreating plot directories...\033[0m\n";
    } else {
        // Physical setup text identifies the adapter/input provenance and the requested accepted-event
        // capacity before the converter begins scanning its source entries.
        std::cout << "\033[33m\nProceeding input arguments...\033[0m\n";
        std::cout << "\033[33mEvent generator:\033[0m " << config.get("event-generator") << " " << config.get("event-generator-version") << '\n';
        std::cout << "\033[33mInputFiles:\033[0m " << config.get("input") << '\n';
        std::cout << "\033[33mLUND file prefix:\033[0m " << config.get("prefix") << '\n';
        std::cout << "\033[33mOutput directory:\033[0m " << output << '\n';
        std::cout << "\033[33mGenerating lundfiles directory:\033[0m " << lund_dir << '\n';
        std::cout << "\033[33mGenerating mchipo directory:\033[0m " << mchipo_dir << '\n';
        std::cout << "\033[33mGenerating reconhipo directory:\033[0m " << recon_dir << '\n';
        std::cout << "\033[33mGenerating monitoring plots directory:\033[0m " << monitoring_dir << '\n';
        std::cout << "\033[33mSaving lundfiles into\033[0m " << lund_dir << '\n';
        std::cout << "\033[33mNumber of events\033[0m " << config.get("events") << '\n';
        std::cout << "\033[33mEvents per output file:\033[0m 10000\n";
    }

    // Shared fields make uniform and physical logs comparable without erasing their source semantics.
    // Target is the geometry key; A and Z remain independent LUND header metadata.
    std::cout << "\033[33mOutput directory:\033[0m " << output << '\n';
    std::cout << "\033[33mLUND directory:\033[0m " << lund_dir << '\n';
    std::cout << "\033[33mOutput prefix:\033[0m " << config.get("prefix") << '\n';
    std::cout << "\033[33mBeam energy [GeV]:\033[0m " << config.get("beam-energy") << '\n';
    std::cout << "\033[33mTarget:\033[0m " << config.get("target") << "  \033[33mA:\033[0m " << config.get("A") << "  \033[33mZ:\033[0m " << config.get("Z") << '\n';
    std::cout << "\033[33mRequested events:\033[0m " << config.get("events") << "  \033[33mEvents per file:\033[0m 10000\n";
    std::cout << "\033[33mLUND format:\033[0m " << config.get("lund-format") << "  \033[33mMass convention:\033[0m " << config.get("mass-convention") << '\n';

    if (final) {
        // Uniform generation writes every generated event, so scanned equals written. Physical scanned
        // includes unsupported interactions skipped before serialization; `written` alone determines
        // the number of 10,000-event LUND files, including a possible partial last file.
        std::cout << "\033[33m\n- Completion summary ----------------------------------------\n\033[0m";
        std::cout << "\033[33mTotal entries scanned:\033[0m " << scanned << '\n';
        std::cout << "\033[33mEvents passing cuts:\033[0m " << written << '\n';
        const auto output_files = (written + 9999) / 10000;
        std::cout << "\033[33mOutput files written:\033[0m " << output_files << '\n';
        std::cout << "\033[33m\nOperation finished!\033[0m\n";
    }

    std::cout << '\n';
}
#pragma endregion

// LundWriter::LundWriter ----------------------------------------------------------------------

#pragma region /* LundWriter::LundWriter */
/**
 * @brief Prepare the run directory and cache output limits.
 *
 * Purpose:
 *   Claim one fully resolved run directory before any event work begins, preserving intentional legacy
 *   replacement while preventing broad or ambiguous recursive deletion targets.
 *
 * Workflow:
 *   1. Borrow the configuration; own the workflow label, output path, capacity, split limit, and format.
 *   2. Convert the final output path to an absolute lexically normalized path.
 *   3. Reject root, home, current-directory, checkout/checkout-ancestor, and otherwise incomplete paths.
 *   4. Create the parent, warn and remove an existing exact run directory, then create `lundfiles/`.
 *
 * @param c Borrowed validated configuration; the stored reference requires it to outlive this writer.
 * @param workflow Source label copied into summaries and manifest provenance (`uniform` or `physical`).
 *
 * @throws std::exception If configuration conversion fails, the path is unsafe, or filesystem creation,
 *         inspection, replacement, or removal fails.
 *
 * @note Replacement is recursive and intentional. RunConfig::parse() has already appended the complete
 *       source-specific run name; this constructor reports and removes only that exact resolved path.
 */
LundWriter::LundWriter(const RunConfig& c, std::string workflow)
    : config_(c), workflow_(std::move(workflow)), directory_(c.get("output")), events_per_file_(10000), capacity_(c.integer("events")), legacy_format_(c.get("lund-format") == "legacy") {
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
        contains_checkout)
        throw std::runtime_error("Refusing unsafe output-directory replacement: " + directory_.string());

    // Ensure the parent exists before checking/replacing the final run. Existing contents at the exact
    // final path are disposable by the documented legacy rerun contract.
    std::filesystem::create_directories(directory_.parent_path());
    if (std::filesystem::exists(directory_)) {
        std::cout << environment::WARNING_COLOR << "Replacing existing run directory (legacy behavior): " << directory_ << environment::RESET_COLOR << '\n';
        std::filesystem::remove_all(directory_);
    }

    // LUND streams are opened lazily by write(); no empty numbered file is created at construction.
    std::filesystem::create_directories(directory_ / "lundfiles");
}
#pragma endregion

// LundWriter::full ----------------------------------------------------------------------

#pragma region /* LundWriter::full */
/**
 * @brief Test whether the configured event capacity has been reached.
 *
 * Purpose:
 *   Give uniform generation and physical conversion the same stopping condition based on successfully
 *   serialized events rather than generated attempts or scanned input entries.
 *
 * @return True when count_ is greater than or equal to the requested capacity_. The greater-than case
 *         is defensive; write() prevents normal operation from advancing beyond capacity.
 *
 * @note Read-only and side-effect free. Physical conversion may scan/reject additional input before
 *       reaching this written-event capacity.
 */
bool LundWriter::full() const { return count_ >= capacity_; }
#pragma endregion

// LundWriter::write ----------------------------------------------------------------------

#pragma region /* LundWriter::write */
/**
 * @brief Serialize one event and advance file and run counts.
 *
 * Purpose:
 *   Keep both generation workflows on one LUND serialization and file-splitting contract.
 *
 * Workflow:
 *   1. Reject capacity overflow and empty events.
 *   2. Lazily open the first numbered file or rotate after exactly events_per_file_ records.
 *   3. Serialize a ten-field event header using legacy-compatible or precise formatting.
 *   4. Derive each particle's on-shell energy and serialize its fourteen-field record in stable order.
 *   5. Increment per-file and run-global counts only after every record has been written.
 *
 * @param e Borrowed event containing beam energy in GeV, particle momentum in GeV/c, mass in GeV/c²,
 *          vertices in cm, explicit A/Z header metadata, and source-defined particle ordering.
 *
 * @return Nothing. Normal return means one complete event was serialized and both counters advanced.
 *
 * @throws std::runtime_error If capacity is exhausted, the particle list is empty, or derived energy/
 *         vertex data is non-finite. The configured stream also throws on open/write/close failures.
 *
 * @note A failure after header output can leave a partial final event in the active file, but counters
 *       do not advance and finish() is not reached by maintained callers, so no completion manifest is
 *       published for consumers.
 */
void LundWriter::write(const Event& e) {
    // Enforce capacity internally even when a caller forgets to check full(). LUND events must contain
    // at least one particle because the header multiplicity and following records form one unit.
    if (full()) throw std::runtime_error("Run file limit reached");
    if (e.particles.empty()) throw std::runtime_error("Cannot write an empty event");

    // Open a new file only when the next event actually needs one. This avoids empty trailing files for
    // exact multiples of 10,000 and allows a partially filled final physical file at end of input.
    if (files_.empty() || files_.back().events == events_per_file_) {
        // Closing the previous stream flushes it before a new manifest record and path are selected.
        if (stream_.is_open()) stream_.close();

        // Number files from one in creation order and store a run-relative path for portable manifests.
        files_.push_back({"lundfiles/" + config_.get("prefix") + "_" + std::to_string(files_.size() + 1) + ".txt", 0});

        // Convert bad/fail stream states into exceptions. Precise output uses ten significant digits;
        // legacy branches below use explicit printf-compatible field precision instead.
        stream_.exceptions(std::ios::badbit | std::ios::failbit);
        stream_.open(directory_ / files_.back().path);
        stream_ << std::setprecision(10);
    }

    // Preserve archived header precision and numbering when legacy output is selected. Uniform IDs
    // restart from zero in each split file; physical IDs retain the source entry index stored in e.id.
    if (legacy_format_) {
        const auto id = static_cast<unsigned long long>(workflow_ == "uniform" ? files_.back().events : e.id);

        // ep/en and the beam-momentum electron tester historically wrote beam energy with one decimal;
        // ordinary 1e and physical conversion used six decimals. Other header fields retain their
        // archived precision and meanings, including physical process tags in e.weight.
        const bool nucleon = workflow_ == "uniform" && config_.get("channel") != "1e";
        const bool tester = workflow_ == "uniform" && config_.get("channel") == "1e" && config_.get("electron-momentum") == "beam";
        const char* format = nucleon || tester ? "%i \t %i \t %i \t %.3f \t %.3f \t %i \t %.1f \t %i \t %llu \t %.3f \n" : "%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %llu \t %.2f \n";
        stream_ << TString::Format(format, static_cast<int>(e.particles.size()), e.A, e.Z, e.resonance_id, 0., 11, e.beam_energy, 1, id, e.weight);
    } else {
        // Precise format keeps one-space separation, the run/source-global ID, and the stream's ten
        // significant-digit precision while preserving the same semantic header fields.
        stream_ << e.particles.size() << ' ' << e.A << ' ' << e.Z << ' ' << e.resonance_id << " 0 11 " << e.beam_energy << " 1 " << e.id << ' ' << e.weight << '\n';
    }

    // Derive mass-shell energy E=sqrt(m²+p²) under c=1 and emit fourteen fields per particle. The loop
    // follows Event::particles order exactly; no source-specific sorting or filtering occurs here.
    int index = 0;
    for (const auto& p : e.particles) {
        const double energy = std::sqrt(p.mass * p.mass + p.momentum.Mag2());

        // A non-finite momentum or mass propagates into energy; checking vertex magnitude catches any
        // non-finite coordinate. Reject before emitting that particle record.
        if (!std::isfinite(energy) || !std::isfinite(p.vertex.Mag2())) throw std::runtime_error("Non-finite particle data");
        if (legacy_format_) {
            // Legacy particle records retain tabs, status/parent zeros, active flag 1, and five decimal
            // places for momentum, energy, mass, and vertex fields.
            stream_ << TString::Format("%i \t %.3f \t %i \t %i \t %i \t %i \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \n", ++index, 0., 1, p.pid, 0, 0, p.momentum.X(),
                                       p.momentum.Y(), p.momentum.Z(), energy, p.mass, p.vertex.X(), p.vertex.Y(), p.vertex.Z());
        } else {
            // Precise records contain the identical fourteen fields and constants with compact spacing.
            stream_ << ++index << " 0 1 " << p.pid << " 0 0 " << p.momentum.X() << ' ' << p.momentum.Y() << ' ' << p.momentum.Z() << ' ' << energy << ' ' << p.mass << ' ' << p.vertex.X()
                    << ' ' << p.vertex.Y() << ' ' << p.vertex.Z() << '\n';
        }
    }

    // Commit bookkeeping only after the full event has reached the stream successfully.
    ++files_.back().events;
    ++count_;
}
#pragma endregion

// LundWriter::finish ----------------------------------------------------------------------

#pragma region /* LundWriter::finish */
/**
 * @brief Publish the completed-run manifest.
 *
 * Purpose:
 *   Mark a run consumable only after its LUND stream and caller-owned diagnostics have completed, while
 *   recording enough software, configuration, target-source, counter, and file provenance to audit or
 *   reproduce the output.
 *
 * Workflow:
 *   1. Close and flush the active LUND stream.
 *   2. Open `manifest.json.tmp` with exceptions enabled.
 *   3. Write schema/software/ROOT/target provenance plus scanned and written counters.
 *   4. Serialize every resolved RunConfig entry and ordered split-file record as strict JSON.
 *   5. Close the temporary file and rename it within the run directory to `manifest.json`.
 *
 * @param scanned Number of source events examined. Uniform generation supplies count(); physical
 *                conversion includes rejected input entries, so scanned may exceed written_events.
 *
 * @return Nothing. Normal return means the final completion manifest is visible to consumers.
 *
 * @throws std::exception If LUND closure, manifest open/write/close, JSON-related allocation, or the
 *         final filesystem rename fails.
 *
 * @pre Required diagnostic files have already been saved successfully. Calling finish() is the final
 *      publication step and maintained workflows call it once.
 *
 * @note A failure can leave `manifest.json.tmp` and partial run data for inspection. Consumers must
 *       require `manifest.json`; the temporary name never declares completion.
 */
void LundWriter::finish(std::uint64_t scanned) {
    // Close first so every numbered LUND file is flushed and no further event can be appended through
    // the active stream while its counts are being published.
    if (stream_.is_open()) stream_.close();

    // Write completion metadata to a temporary sibling so consumers never observe a partially written
    // final manifest. Stream exceptions turn open, serialization, flush, and close failures into the
    // same workflow error path used by LUND output.
    std::ofstream manifest;
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    manifest.open(directory_ / "manifest.json.tmp");

    // Fixed top-level provenance identifies the manifest schema, this project build/revision, the ROOT
    // runtime, and the exact protected targets.h content compiled into the application. count_ records
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
    // visibility boundary: only after it succeeds does manifest.json advertise a completed run.
    manifest.close();
    std::filesystem::rename(directory_ / "manifest.json.tmp", directory_ / "manifest.json");
}
#pragma endregion

}  // namespace samples
