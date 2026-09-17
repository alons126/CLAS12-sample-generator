//
// Created by Alon Sportes on 14/09/2026.
//

/**
 * @file Monitoring.h
 * @brief Declares the shared, run-local ROOT monitoring interface.
 *
 * Purpose:
 *   Observe the particles that the LUND workflow writes and collect a common
 *   set of per-species kinematic and vertex diagnostics. The monitor does not
 *   select events, alter particles, or participate in LUND serialization.
 *
 * Workflow:
 *   1. Construct one Monitoring object for a LUND-creation run, supplying the
 *      beam energy used to size its momentum axes.
 *   2. Pass each successfully written Event to fill(). Histogram families are
 *      created lazily for the PDG codes that actually occur.
 *   3. Call save() after the event loop to create the run's ROOT diagnostics.
 *
 * Units and assumptions:
 *   Particle momentum is expressed in GeV/c, angular plots use degrees, vertex
 *   coordinates use centimeters, and the constructor's beam energy is in GeV.
 *   Event-source-specific monitoring may exist alongside this common view.
 *
 * Ownership and failure behavior:
 *   Monitoring exclusively owns its private histogram implementation. ROOT
 *   types remain hidden from users of this header. save() reports file creation
 *   and histogram write failures by throwing std::runtime_error.
 */

#pragma once
#include <filesystem>
#include <memory>

#include "common/Event.h"

namespace samples {

// Public interface ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Public interface */

// Monitoring object -----------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Monitoring object */
/**
 * @class Monitoring
 * @brief Owns the common diagnostic histograms for one LUND-creation run.
 *
 * Purpose:
 *   Provide both uniform and physical event sources with the same basic view
 *   of the particles that reached the LUND output.
 *
 * Usage:
 *   Construct with the configured beam energy, call fill() once for every
 *   successfully written event, and call save() after generation completes.
 *
 * State and ownership:
 *   A private implementation owns one detached ROOT histogram family per
 *   observed PDG code. The pImpl boundary prevents ROOT histogram declarations
 *   from becoming part of this public header. The object is consequently
 *   non-copyable, and its resources are released with the Monitoring instance.
 *
 * Invariants:
 *   The beam energy must be a valid positive run configuration value before
 *   construction. fill() only observes its Event argument; it neither retains
 *   a reference to the event nor changes the event. Histogram accumulation is
 *   intended for a single sequential run.
 */
class Monitoring {
   public:
    /**
     * @brief Start an empty set of run-local monitoring histograms.
     *
     * Histogram families are not allocated until fill() encounters a particle.
     *
     * @param beam_energy Configured beam energy in GeV; used to set momentum-axis ranges.
     */
    explicit Monitoring(double beam_energy);

    /**
     * @brief Release the implementation and all detached ROOT histograms it owns.
     *
     * The destructor is defined out of line so Impl may remain incomplete here.
     */
    ~Monitoring();

    /**
     * @brief Add every particle in one written event to the common diagnostics.
     *
     * For each particle, the implementation records momentum magnitude in
     * GeV/c, theta and phi in degrees, vertex coordinates in centimeters, and
     * the theta-versus-phi, theta-versus-momentum, and phi-versus-momentum
     * correlations. A histogram family is created on the first occurrence of
     * each PDG code.
     *
     * @param event Successfully written event to observe; ownership remains with the caller.
     *
     * @note This function does not filter or modify the event.
     */
    void fill(const Event& event);

    /**
     * @brief Persist all accumulated histogram families in a new ROOT file.
     *
     * @param path Destination path supplied by the run workflow.
     *
     * @throws std::runtime_error If ROOT cannot create the file or write a histogram.
     * @note ROOT's CREATE mode treats an existing destination as a creation failure.
     */
    void save(const std::filesystem::path& path);

    // Owned state -------------------------------------------------------------------------------------------------------------------------------------------------------
   private:
    /**
     * @struct Impl
     * @brief ROOT-backed storage defined in Monitoring.cpp.
     *
     * It stores the beam-axis scale and the lazily created, PDG-keyed histogram
     * families. Keeping the record private isolates callers from ROOT details.
     */
    struct Impl;

    std::unique_ptr<Impl> impl_;  ///< Exclusive run-long ownership of monitoring state.
};
#pragma endregion

#pragma endregion

}  // namespace samples
