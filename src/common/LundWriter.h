#pragma once
#include <fstream>
#include <vector>

#include "common/Event.h"
#include "common/RunConfig.h"
namespace samples {
// A run is consumable only after finish() publishes manifest.json.
class LundWriter {
   public:
    LundWriter(const RunConfig& config, std::string workflow);
    bool full() const;
    void write(const Event& event);
    void finish(std::uint64_t scanned);
    std::uint64_t count() const { return count_; }

   private:
    struct Output {
        std::string path;
        std::uint64_t events = 0;
    };
    const RunConfig& config_;
    std::string workflow_;
    std::filesystem::path directory_;
    std::ofstream stream_;
    std::vector<Output> files_;
    std::uint64_t count_ = 0;
    std::uint64_t events_per_file_, capacity_;
    bool legacy_format_;
};
}  // namespace samples
