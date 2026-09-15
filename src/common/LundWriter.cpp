/**
 * @file LundWriter.cpp
 * @brief LUND serialization and completed-run manifests.
 *
 * Purpose:
 *   Write shared event records in legacy or precise format without overwriting existing runs.
 *
 * Workflow:
 *   Claim directory -> rotate files as needed -> write particles -> close -> publish manifest.
 */

#include "common/LundWriter.h"

#include <TROOT.h>
#include <TString.h>

#include <cmath>
#include <iomanip>
#include <stdexcept>

#include "Version.h"
namespace samples {
// LundWriter::LundWriter ----------------------------------------------------------------------

#pragma region /* LundWriter::LundWriter */
/**
 * @brief Claim a new run directory and cache output limits.
 *
 * Algorithm:
 *   Cache settings, create parent directories, then exclusively create the run and LUND directory.
 *
 * @param c Validated configuration; must outlive this writer.
 * @param workflow Manifest workflow label, uniform or genie.
 *
 * @note Constructor; existing run directories and filesystem failures throw.
 */
LundWriter::LundWriter(const RunConfig& c, std::string workflow)
    : config_(c),
      workflow_(std::move(workflow)),
      directory_(c.get("output")),
      events_per_file_(c.integer("events-per-file")),
      capacity_(c.integer("files") * events_per_file_),
      legacy_format_(c.get("lund-format") == "legacy") {
    // Atomic leaf creation prevents two runs from claiming the same directory.
    std::filesystem::create_directories(directory_.parent_path());
    if (!std::filesystem::create_directory(directory_)) throw std::runtime_error("Output already exists: " + directory_.string());
    std::filesystem::create_directory(directory_ / "lundfiles");
}
#pragma endregion

// LundWriter::full ----------------------------------------------------------------------

#pragma region /* LundWriter::full */
/**
 * @brief Test whether the configured event capacity has been reached.
 *
 * Algorithm:
 *   Compare the written count with files multiplied by events per file.
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
 * @param e Event containing metadata and particles in GeV and cm.
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
