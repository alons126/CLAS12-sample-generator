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
void TargetGeometry::validate(const std::string& name) {
    // The point vertex is an artificial electron-tester setting, not a target.
    if (name == "point") return;
    const auto found = external_targets::targets.find(name);
    if (found == external_targets::targets.end() || found->second.empty())
        throw std::runtime_error("Unknown or empty target geometry in targets.h: " + name);
}
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
}
