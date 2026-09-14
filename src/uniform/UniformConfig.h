#pragma once
#include "common/RunConfig.h"
namespace samples {
enum class UniformChannel { Electron, ElectronProton, ElectronNeutron };
// Resolve string settings once, outside the production event loop.
struct UniformConfig {
    UniformChannel channel;
    bool uniform_electron_momentum, uniform_nucleon_momentum, mixed_nucleon_momentum, isotropic_nucleon_angle;
    double beam, electron_theta_min, electron_theta_max;
    double nucleon_theta_min, nucleon_theta_max, nucleon_p, nucleon_p_min, nucleon_p_max;
    double trigger_theta, trigger_phi_offset;
    int A, Z;
    explicit UniformConfig(const RunConfig& c)
        : channel(c.get("channel") == "1e"   ? UniformChannel::Electron
                  : c.get("channel") == "ep" ? UniformChannel::ElectronProton
                                             : UniformChannel::ElectronNeutron),
          uniform_electron_momentum(c.get("electron-momentum") == "uniform"),
          uniform_nucleon_momentum(c.get("nucleon-momentum") == "uniform"),
          mixed_nucleon_momentum(c.get("nucleon-momentum") == "mixed"),
          isotropic_nucleon_angle(c.get("nucleon-angle") == "isotropic"),
          beam(c.number("beam-energy")),
          electron_theta_min(c.number("electron-theta-min")),
          electron_theta_max(c.number("electron-theta-max")),
          nucleon_theta_min(c.number("nucleon-theta-min")),
          nucleon_theta_max(c.number("nucleon-theta-max")),
          nucleon_p(c.number("nucleon-p")),
          nucleon_p_min(c.number("nucleon-p-min")),
          nucleon_p_max(c.number("nucleon-p-max")),
          trigger_theta(c.number("trigger-theta")),
          trigger_phi_offset(c.number("trigger-phi-offset")),
          A(static_cast<int>(c.integer("A"))),
          Z(static_cast<int>(c.integer("Z"))) {}
};
}  // namespace samples
