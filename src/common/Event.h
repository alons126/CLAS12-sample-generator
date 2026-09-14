#pragma once
#include <TVector3.h>

#include <cstdint>
#include <vector>
namespace samples {
struct Particle {
    int pid;
    double mass;
    TVector3 momentum;
    TVector3 vertex;
};
struct Event {
    std::uint64_t id = 0;
    int A = 1, Z = 1;
    double beam_energy = 0, resonance_id = 0, weight = 1;
    std::vector<Particle> particles;
};
double particleMass(int pid);
}  // namespace samples
