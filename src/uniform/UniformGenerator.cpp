#include "uniform/UniformGenerator.h"

#include <TMath.h>

#include <cmath>
#include <iostream>

#include "common/LundWriter.h"
#include "common/Monitoring.h"
#include "common/TargetGeometry.h"
#include "uniform/UniformConfig.h"
namespace samples {
namespace {
TVector3 momentum(double p, double theta, double phi) {
    TVector3 v;
    v.SetMagThetaPhi(p, theta * TMath::DegToRad(), phi * TMath::DegToRad());
    return v;
}
double triggerPhi(double phi, double offset) {
    auto wrap = [](double angle) { return angle > 180 ? angle - 360 : angle < -180 ? angle + 360 : angle; };
    const double target = wrap(phi + 180);
    double closest = -120, difference = std::abs(wrap(target - closest));
    for (double angle : {-60., 0., 60., 120., 180.}) {
        double d = std::abs(wrap(target - angle));
        if (d < difference) {
            difference = d;
            closest = angle;
        }
    }
    return closest + offset;
}
}  // namespace
void generateUniform(const RunConfig& c) {
    c.validate(false);
    const UniformConfig settings(c);
    const auto channel = settings.channel;
    TRandom3 random(c.integer("seed")), vertex_random(c.integer("vertex-seed"));
    TargetGeometry geometry(c.get("target"));
    LundWriter writer(c, "uniform");
    const double beam = settings.beam;
    Monitoring monitoring(beam);
    while (!writer.full()) {
        Event event;
        event.id = writer.count();
        event.A = settings.A;
        event.Z = settings.Z;
        event.beam_energy = beam;
        const auto vertex = geometry.sample(vertex_random);
        if (channel == UniformChannel::Electron) {
            double theta = random.Uniform(settings.electron_theta_min, settings.electron_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_electron_momentum ? random.Uniform(0, beam) : beam;
            event.particles.push_back({11, particleMass(11), momentum(p, theta, phi), vertex});
        } else {
            double theta = random.Uniform(settings.nucleon_theta_min, settings.nucleon_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_nucleon_momentum ? random.Uniform(settings.nucleon_p_min, settings.nucleon_p_max) : settings.nucleon_p;
            int pid = channel == UniformChannel::ElectronProton ? 2212 : 2112;
            event.particles.push_back({11, particleMass(11), momentum(beam, settings.trigger_theta, triggerPhi(phi, settings.trigger_phi_offset)), vertex});
            event.particles.push_back({pid, particleMass(pid), momentum(p, theta, phi), vertex});
        }
        writer.write(event);
        monitoring.fill(event);
    }
    monitoring.save(std::filesystem::path(c.get("output")) / "monitoring.root");
    writer.finish(writer.count());
    std::cout << "Wrote " << writer.count() << " events to " << c.get("output") << '\n';
}
}  // namespace samples
