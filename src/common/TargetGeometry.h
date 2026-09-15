/**
 * @file TargetGeometry.h
 * @brief Seeded target-vertex adapter interface.
 *
 * Purpose:
 *   Expose target validation and vertex sampling without exposing external globals.
 *
 * Workflow:
 *   Construct with a target name; pass the run-owned vertex RNG to sample for each event.
 */

#pragma once
#include <TRandom3.h>
#include <TVector3.h>

#include <string>
#include <utility>
namespace samples {

// Public interface -------------------------------------------------------------
#pragma region /* Public interface */

// TargetGeometry object ------------------------------------------------
#pragma region /* TargetGeometry object */
/**
 * @class TargetGeometry
 * @brief Bridge between a run-owned vertex stream and protected targets.h.
 *
 * Usage order: validate a name at construction, then sample once per event.
 * Physical geometry comes from the external header; point is an artificial origin.
 * The adapter isolates the external global RNG and preserves each caller's state.
 */
class TargetGeometry {
   public:
    /** @brief Store and validate the selected target name without consuming random draws. */
    explicit TargetGeometry(std::string name) : name_(std::move(name)) { validate(name_); }
    /** @brief Require point or a nonempty target-map entry; unknown names throw. */
    static void validate(const std::string& name);
    /** @brief Return a vertex in cm and advance random; point consumes no draws. */
    TVector3 sample(TRandom3& random) const;

    // Owned state --------------------------------------------------------------
   private:
    std::string name_;  ///< Validated external map key, or the artificial point mode.
};
#pragma endregion
#pragma endregion
}  // namespace samples
