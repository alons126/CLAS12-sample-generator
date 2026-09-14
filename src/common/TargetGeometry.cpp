#include "common/TargetGeometry.h"

#include <map>
#include <stdexcept>
#include <vector>

#include "common/Event.h"
namespace samples {
namespace {
const std::map<std::string, std::vector<double>> positions = {{"4-foil", {-4.875, -3.625, -2.375, -1.125}},
                                                              {"1-foil", {-0.5}},
                                                              {"1-foil-small", {-2.1}},
                                                              {"1-foil-large", {-2.32}},
                                                              {"Ar", {-5.75, -5.25}},
                                                              {"Ca", {-3.0}},
                                                              {"liquid", {-5.5, -0.5}},
                                                              {"point", {0.0}}};
}
void TargetGeometry::validate(const std::string& name) {
    if (!positions.count(name)) throw std::runtime_error("Unknown target geometry: " + name);
}
TVector3 TargetGeometry::sample(TRandom3& random) const {
    if (name_ == "point") return {0, 0, 0};
    const auto& z = positions.at(name_);
    double x = random.Gaus(0, 0.04), y = random.Gaus(0, 0.04);
    double vz = (name_ == "Ar" || name_ == "liquid") ? random.Uniform(z[0], z[1]) : z[random.Integer(z.size())];
    return {x, y, vz};
}
double particleMass(int pid, bool legacy) {
    switch (pid) {
        case 11:
            return 0.000511;
        case 2212:
            return 0.938272;
        case 2112:
            return 0.93957;
        case 211:
        case -211:
            return legacy ? 0.13957 : 0.13957039;
        case 111:
            return legacy ? 0.13957 : 0.1349768;
        case 22:
            return 0;
        default:
            throw std::runtime_error("Unsupported output PDG code: " + std::to_string(pid));
    }
}
}  // namespace samples
