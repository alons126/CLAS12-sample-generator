#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include "common/Event.h"
namespace samples {
// Original uniform histogram names, binning and correlations, owned per run.
class LegacyMonitoring {
   public:
    LegacyMonitoring(const std::string& channel, double beam);
    ~LegacyMonitoring();
    void fill(const Event& event);
    void save(const std::filesystem::path& path, bool render);

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace samples
