//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file LundWriter.cpp
 * @brief LUND serialization and completed-run manifests.
 *
 * Purpose:
 *   Write shared event records in legacy or precise format, replacing stale output directories on rerun.
 *
 * Workflow:
 *   Claim directory -> rotate files as needed -> write particles -> close -> publish manifest.
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

#include "common/environment.h"
#include "Version.h"
namespace samples {
// LundWriter::printWorkflowSummary ----------------------------------------------------------------------

#pragma region /* LundWriter::printWorkflowSummary */
/**
 * @brief Print the legacy-style setup and completion summary for a LUND run.
 *
 * Algorithm:
 *   Print the shared output metadata, followed by workflow-specific settings and per-run totals.
 *
 * @param config Resolved workflow configuration.
 * @param workflow Manifest workflow label, uniform or physical.
 * @param scanned Number of input entries scanned for the final summary.
 * @param written Number of accepted/written events in the final summary.
 * @param final True when printing the completion report; false for setup information.
 */
void LundWriter::printWorkflowSummary(const RunConfig& config, const std::string& workflow, std::uint64_t scanned, std::uint64_t written, bool final) {
    const auto output = std::filesystem::path(config.get("output"));
    const auto lund_dir = output / "lundfiles";
    const auto mchipo_dir = output / "mchipo";
    const auto recon_dir = output / "reconhipo";
    const auto rootfiles_dir = output / "rootfiles";
    const auto monitoring_dir = output / "monitoring_plots";
    const bool uniform = workflow == "uniform";

    std::cout << "\033[33m\n=============================================================\n\033[0m";
    std::cout << "\033[33m\n= " << (uniform ? "Uniform sample generation" : "Physical generator to LUND conversion") << " summary" << "\n\033[0m";
    std::cout << "\033[33m=============================================================\n\033[0m";

    if (uniform) {
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
        std::cout << "\033[33mChannel:\033[0m " << config.get("channel") << "  \033[33mElectron momentum:\033[0m " << config.get("electron-momentum")
                  << "  \033[33mNucleon momentum:\033[0m " << config.get("nucleon-momentum") << '\n';
        std::cout << "\033[33mtargP:\033[0m 0  \033[33mbeamP:\033[0m 0  \033[33minteractN:\033[0m 1  \033[33mbeamType:\033[0m 11\n";
        std::cout << "\033[33mbeamE_in_lundfiles:\033[0m " << config.get("beam-energy") << "\n";
        std::cout << "\033[33mweight:\033[0m 1\n";
        std::cout << "\033[33mCreating plot directories...\033[0m\n";
    } else {
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

    std::cout << "\033[33mOutput directory:\033[0m " << output << '\n';
    std::cout << "\033[33mLUND directory:\033[0m " << lund_dir << '\n';
    std::cout << "\033[33mOutput prefix:\033[0m " << config.get("prefix") << '\n';
    std::cout << "\033[33mBeam energy [GeV]:\033[0m " << config.get("beam-energy") << '\n';
    std::cout << "\033[33mTarget:\033[0m " << config.get("target") << "  \033[33mA:\033[0m " << config.get("A") << "  \033[33mZ:\033[0m " << config.get("Z") << '\n';
    std::cout << "\033[33mRequested events:\033[0m " << config.get("events") << "  \033[33mEvents per file:\033[0m 10000\n";
    std::cout << "\033[33mLUND format:\033[0m " << config.get("lund-format") << "  \033[33mMass convention:\033[0m " << config.get("mass-convention") << '\n';

    if (final) {
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
 * Algorithm:
 *   Cache settings, warn before removing an existing run directory, then create the run and LUND directory.
 *
 * @param c Validated configuration; must outlive this writer.
 * @param workflow Manifest workflow label, uniform or physical.
 *
 * @note Constructor; previous output is intentionally removed before the new run starts.
 */
LundWriter::LundWriter(const RunConfig& c, std::string workflow)
    : config_(c),
      workflow_(std::move(workflow)),
      directory_(c.get("output")),
    events_per_file_(10000),
    capacity_(c.integer("events")),
      legacy_format_(c.get("lund-format") == "legacy") {
    directory_ = std::filesystem::absolute(directory_).lexically_normal();
    const auto root = directory_.root_path();
    const auto source = std::filesystem::path(SAMPLE_SOURCE_DIR).lexically_normal();
    const char* home_value = std::getenv("HOME");
    const auto home = home_value ? std::filesystem::path(home_value).lexically_normal() : std::filesystem::path();
    const auto directory_text = directory_.string() + std::filesystem::path::preferred_separator;
    const bool contains_checkout = source == directory_ || source.string().rfind(directory_text, 0) == 0;
    if (directory_.empty() || directory_ == root || (!home.empty() && directory_ == home) || directory_.filename().empty() || directory_ == std::filesystem::current_path() || contains_checkout)
        throw std::runtime_error("Refusing unsafe output-directory replacement: " + directory_.string());
    std::filesystem::create_directories(directory_.parent_path());
    if (std::filesystem::exists(directory_)) {
        std::cout << environment::WARNING_COLOR << "Replacing existing run directory (legacy behavior): " << directory_ << environment::RESET_COLOR << '\n';
        std::filesystem::remove_all(directory_);
    }
    std::filesystem::create_directories(directory_ / "lundfiles");
}
#pragma endregion

// LundWriter::full ----------------------------------------------------------------------

#pragma region /* LundWriter::full */
/**
 * @brief Test whether the configured event capacity has been reached.
 *
 * Algorithm:
 *   Compare the written count with the requested total event count.
 *
 * @return True when no further event may be written.
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
 * Algorithm:
 *   1. Reject capacity overflow and empty events.
 *   2. Rotate to a new file only when needed.
 *   3. Write the selected header format and mass-shell particle records.
 *   4. Advance counts after successful serialization.
 *
 * @param e Event containing beam energy in GeV, particle momentum in GeV/c, mass in GeV/c², and vertices in cm.
 *
 * @note No value; output failures leave partial files for inspection.
 */
void LundWriter::write(const Event& e) {
    if (full()) throw std::runtime_error("Run file limit reached");
    if (e.particles.empty()) throw std::runtime_error("Cannot write an empty event");
    // Open a new file only when the next event actually needs one.
    if (files_.empty() || files_.back().events == events_per_file_) {
        if (stream_.is_open()) stream_.close();
        files_.push_back({"lundfiles/" + config_.get("prefix") + "_" + std::to_string(files_.size() + 1) + ".txt", 0});
        stream_.exceptions(std::ios::badbit | std::ios::failbit);
        stream_.open(directory_ / files_.back().path);
        stream_ << std::setprecision(10);
    }
    // Preserve archived header precision and numbering when legacy output is selected.
    if (legacy_format_) {
        const auto id = static_cast<unsigned long long>(workflow_ == "uniform" ? files_.back().events : e.id);
        const bool nucleon = workflow_ == "uniform" && config_.get("channel") != "1e";
        const bool tester = workflow_ == "uniform" && config_.get("channel") == "1e" && config_.get("electron-momentum") == "beam";
        const char* format = nucleon || tester ? "%i \t %i \t %i \t %.3f \t %.3f \t %i \t %.1f \t %i \t %llu \t %.3f \n" : "%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %llu \t %.2f \n";
        stream_ << TString::Format(format, static_cast<int>(e.particles.size()), e.A, e.Z, e.resonance_id, 0., 11, e.beam_energy, 1, id, e.weight);
    } else {
        stream_ << e.particles.size() << ' ' << e.A << ' ' << e.Z << ' ' << e.resonance_id << " 0 11 " << e.beam_energy << " 1 " << e.id << ' ' << e.weight << '\n';
    }
    // Derive mass-shell energies and emit fourteen fields per particle.
    int index = 0;
    for (const auto& p : e.particles) {
        const double energy = std::sqrt(p.mass * p.mass + p.momentum.Mag2());
        if (!std::isfinite(energy) || !std::isfinite(p.vertex.Mag2())) throw std::runtime_error("Non-finite particle data");
        if (legacy_format_) {
            stream_ << TString::Format("%i \t %.3f \t %i \t %i \t %i \t %i \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \t %.5f \n", ++index, 0., 1, p.pid, 0, 0, p.momentum.X(),
                                       p.momentum.Y(), p.momentum.Z(), energy, p.mass, p.vertex.X(), p.vertex.Y(), p.vertex.Z());
        } else {
            stream_ << ++index << " 0 1 " << p.pid << " 0 0 " << p.momentum.X() << ' ' << p.momentum.Y() << ' ' << p.momentum.Z() << ' ' << energy << ' ' << p.mass << ' ' << p.vertex.X()
                    << ' ' << p.vertex.Y() << ' ' << p.vertex.Z() << '\n';
        }
    }
    ++files_.back().events;
    ++count_;
}
#pragma endregion

// LundWriter::finish ----------------------------------------------------------------------

#pragma region /* LundWriter::finish */
/**
 * @brief Publish the completed-run manifest.
 *
 * Algorithm:
 *   Close LUND output; write settings, provenance and counts to a temporary manifest; rename it into place.
 *
 * @param scanned Number of input events examined, including events rejected by conversion.
 *
 * @note No value; call only after diagnostic outputs have succeeded.
 */
void LundWriter::finish(std::uint64_t scanned) {
    if (stream_.is_open()) stream_.close();
    // Write completion metadata to a temporary file so consumers never see a partial manifest.
    std::ofstream manifest;
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    manifest.open(directory_ / "manifest.json.tmp");
    manifest << "{\n  \"schema_version\": 1,\n  \"workflow\": " << jsonString(workflow_) << ",\n  \"version\": " << jsonString(SAMPLE_VERSION)
             << ",\n  \"revision\": " << jsonString(SAMPLE_REVISION) << ",\n  \"root_version\": " << jsonString(gROOT->GetVersion()) << ",\n  \"targets_sha256\": " << jsonString(SAMPLE_TARGETS_SHA256) << ",\n  \"scanned_events\": " << scanned
             << ",\n  \"written_events\": " << count_ << ",\n  \"config\": {";
    bool first = true;
    for (const auto& [k, v] : config_.values()) {
        manifest << (first ? "\n" : ",\n") << "    " << jsonString(k) << ": " << jsonString(v);
        first = false;
    }
    manifest << "\n  },\n  \"files\": [";
    first = true;
    for (const auto& file : files_) {
        manifest << (first ? "\n" : ",\n") << "    {\"path\": " << jsonString(file.path) << ", \"events\": " << file.events << '}';
        first = false;
    }
    manifest << "\n  ]\n}\n";
    manifest.close();
    std::filesystem::rename(directory_ / "manifest.json.tmp", directory_ / "manifest.json");
}
#pragma endregion

}  // namespace samples
