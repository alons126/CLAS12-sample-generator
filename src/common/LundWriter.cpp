#include "common/LundWriter.h"

#include <TROOT.h>

#include <cmath>
#include <iomanip>
#include <stdexcept>

#include "Version.h"
namespace samples {
LundWriter::LundWriter(const RunConfig& c, std::string workflow)
    : config_(c), workflow_(std::move(workflow)), directory_(c.get("output")), events_per_file_(c.integer("events-per-file")), capacity_(c.integer("files") * events_per_file_) {
    // Atomic leaf creation prevents two runs from claiming the same directory.
    std::filesystem::create_directories(directory_.parent_path());
    if (!std::filesystem::create_directory(directory_)) throw std::runtime_error("Output already exists: " + directory_.string());
    std::filesystem::create_directory(directory_ / "lundfiles");
}
bool LundWriter::full() const { return count_ >= capacity_; }
void LundWriter::write(const Event& e) {
    if (full()) throw std::runtime_error("Run file limit reached");
    if (e.particles.empty()) throw std::runtime_error("Cannot write an empty event");
    if (files_.empty() || files_.back().events == events_per_file_) {
        if (stream_.is_open()) stream_.close();
        files_.push_back({"lundfiles/" + config_.get("prefix") + "_" + std::to_string(files_.size() + 1) + ".txt", 0});
        stream_.exceptions(std::ios::badbit | std::ios::failbit);
        stream_.open(directory_ / files_.back().path);
        stream_ << std::setprecision(10);
    }
    stream_ << e.particles.size() << ' ' << e.A << ' ' << e.Z << ' ' << e.resonance_id << " 0 11 " << e.beam_energy << " 1 " << e.id << ' ' << e.weight << '\n';
    int index = 0;
    for (const auto& p : e.particles) {
        const double energy = std::sqrt(p.mass * p.mass + p.momentum.Mag2());
        if (!std::isfinite(energy) || !std::isfinite(p.vertex.Mag2())) throw std::runtime_error("Non-finite particle data");
        stream_ << ++index << " 0 1 " << p.pid << " 0 0 " << p.momentum.X() << ' ' << p.momentum.Y() << ' ' << p.momentum.Z() << ' ' << energy << ' ' << p.mass << ' ' << p.vertex.X() << ' '
                << p.vertex.Y() << ' ' << p.vertex.Z() << '\n';
    }
    ++files_.back().events;
    ++count_;
}
void LundWriter::finish(std::uint64_t scanned) {
    if (stream_.is_open()) stream_.close();
    std::ofstream manifest;
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    manifest.open(directory_ / "manifest.json.tmp");
    manifest << "{\n  \"schema_version\": 1,\n  \"workflow\": " << jsonString(workflow_) << ",\n  \"version\": " << jsonString(SAMPLE_VERSION)
             << ",\n  \"revision\": " << jsonString(SAMPLE_REVISION) << ",\n  \"root_version\": " << jsonString(gROOT->GetVersion()) << ",\n  \"scanned_events\": " << scanned
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
}  // namespace samples
