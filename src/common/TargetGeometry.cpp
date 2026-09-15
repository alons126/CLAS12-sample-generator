/**
 * @file TargetGeometry.cpp
 * @brief Adapter around the protected external target source.
 *
 * Purpose:
 *   Use targets.h as the geometry authority while keeping run RNG streams independent.
 *
 * Workflow:
 *   Validate the target map entry; transfer RNG state; call randomVertex; restore caller state.
 */

#include "common/TargetGeometry.h"

#include <TString.h>
#include <cmath>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace {
// This external header defines globals and functions. Include it in ONE translation
// unit, isolated from application symbols. Keep the header itself replaceable.
namespace external_targets {
using std::cout;
using std::endl;
using std::sqrt;
using std::string;
#include "common/targets.h"
}
std::mutex geometry_mutex;
}

namespace samples {
// TargetGeometry::validate ----------------------------------------------------------------------

#pragma region /* TargetGeometry::validate */
/**
 * @brief Validate a target name without sampling.
 *
 * Algorithm:
 *   Accept the artificial point mode; otherwise require a nonempty external map entry.
 *
 * @param name Target key in targets.h, or point.
 *
 * @note No value; throws for unknown or empty target entries.
 */
void TargetGeometry::validate(const std::string& name) {
    // The point vertex is an artificial electron-tester setting, not a target.
    if (name == "point") return;
    const auto found = external_targets::targets.find(name);
    if (found == external_targets::targets.end() || found->second.empty())
        throw std::runtime_error("Unknown or empty target geometry in targets.h: " + name);
}
#pragma endregion

// TargetGeometry::sample ----------------------------------------------------------------------

#pragma region /* TargetGeometry::sample */
/**
 * @brief Sample one vertex using the caller-owned random stream.
 *
 * Purpose:
 *   Use the external geometry algorithm without coupling otherwise independent run RNG streams.
 *
 * Algorithm:
 *   1. Return the origin directly for point mode.
 *   2. Lock the external RNG and copy in the caller state.
 *   3. Call randomVertex, copy the advanced state back, and check the vertex.
 *
 * @param random Vertex RNG whose state advances for physical targets.
 *
 * @return Vertex in cm; throws if the external sampler returns non-finite coordinates.
 */
TVector3 TargetGeometry::sample(TRandom3& random) const {
    if (name_ == "point") return {0, 0, 0};
    // Upstream randomVertex uses a global TRandom3 named ran. Transfer the full
    // caller-owned state, not just its seed, so streams remain reproducible and
    // independent even when different geometry instances are interleaved.
    std::lock_guard<std::mutex> guard(geometry_mutex);
    external_targets::ran = random;
    const auto vertex = external_targets::randomVertex(name_);
    random = external_targets::ran;
    if (!std::isfinite(vertex.Mag2())) throw std::runtime_error("Non-finite vertex from targets.h");
    return vertex;
}
#pragma endregion

}
