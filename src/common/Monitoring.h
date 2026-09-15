/**
 * @file Monitoring.h
 * @brief Common per-particle diagnostic interface.
 *
 * Purpose:
 *   Keep ROOT histogram ownership local to a run.
 *
 * Workflow:
 *   Construct with beam energy; fill each written event; save before publishing the manifest.
 */

#pragma once
#include <filesystem>
#include <memory>

#include "common/Event.h"
namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// Owns ROOT histograms without adding global objects to generator code.
// Monitoring object ------------------------------------------------
#pragma region /* Monitoring object */
/**
 * @class Monitoring
 * @brief Run-owned common diagnostic histograms grouped by PDG code.
 *
 * Usage order: construct with beam energy -> fill written events -> save a ROOT file.
 * Histogram allocation is lazy; detached ROOT objects are released with this instance.
 */
class Monitoring {
   public:
    /** @brief Store beam energy in GeV for diagnostic axis ranges. */
    explicit Monitoring(double beam_energy);
    /** @brief Release owned ROOT histograms through the private implementation. */
    ~Monitoring();
    /** @brief Accumulate momentum, angle, vertex and correlation histograms. */
    void fill(const Event& event);
    /** @brief Create a diagnostic ROOT file; throw on creation or write failure. */
    void save(const std::filesystem::path& path);

    // Owned state --------------------------------------------------------------
   private:
    /** @brief Private histogram storage defined in the .cpp; owned exclusively by impl_. */
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#pragma endregion
#pragma endregion
}  // namespace samples
