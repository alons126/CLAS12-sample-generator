#include "common/Event.h"
#include <stdexcept>
#include <string>
namespace samples {
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
}
