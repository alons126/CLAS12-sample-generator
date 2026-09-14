#pragma once
#include <filesystem>
#include <memory>

#include "common/Event.h"
namespace samples {
// Owns ROOT histograms without adding global objects to generator code.
class Monitoring {
   public:
    explicit Monitoring(double beam_energy);
    ~Monitoring();
    void fill(const Event& event);
    void save(const std::filesystem::path& path);

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace samples
