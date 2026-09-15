/**
 * @file LegacyMonitoring.h
 * @brief Maintained compatibility-diagnostic interface.
 *
 * Purpose:
 *   Provide the historical uniform histogram names and correlations with run-local ownership.
 *
 * Workflow:
 *   Choose a channel at construction; fill written events; save histograms and optional plots.
 */

#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include "common/Event.h"

namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// Original uniform histogram names, binning and correlations, owned per run.
// LegacyMonitoring object ------------------------------------------------
#pragma region /* LegacyMonitoring object */
/**
 * @class LegacyMonitoring
 * @brief Maintained channel diagnostics matching the archived numerical definitions.
 *
 * Usage order: select channel and beam -> fill written events -> save, optionally render.
 * This class owns its histograms; it does not import or modify archived source files.
 */
class LegacyMonitoring {
   public:
    /** @brief Allocate definitions for 1e, ep, en or Tester_e using beam in GeV. */
    LegacyMonitoring(const std::string& channel, double beam);
    /** @brief Destroy all run-owned historical diagnostic histograms. */
    ~LegacyMonitoring();
    /** @brief Resolve the configured particle quantities and fill all channel histograms. */
    void fill(const Event& event);
    /** @brief Write ROOT histograms and, when render is true, PDF/PNG visualizations. */
    void save(const std::filesystem::path& path, bool render);

    // Owned state --------------------------------------------------------------
   private:
    /** @brief Private histogram storage defined in the .cpp; owned exclusively by impl_. */
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#pragma endregion
#pragma endregion
}  // namespace samples
