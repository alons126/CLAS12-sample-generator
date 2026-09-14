#include "uniform/UniformGenerator.h"

#include <TMath.h>

#include <cmath>
#include <iostream>

#include "common/LegacyMonitoring.h"
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
    LegacyMonitoring legacy_monitoring(c.get("channel") == "1e" && c.get("electron-momentum") == "beam" ? "Tester_e" : c.get("channel"), beam);
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
            double theta =
                settings.isotropic_nucleon_angle
                    ? std::acos(random.Uniform(std::cos(settings.nucleon_theta_max * TMath::DegToRad()), std::cos(settings.nucleon_theta_min * TMath::DegToRad()))) * TMath::RadToDeg()
                    : random.Uniform(settings.nucleon_theta_min, settings.nucleon_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_nucleon_momentum ? random.Uniform(settings.nucleon_p_min, settings.nucleon_p_max) : settings.nucleon_p;
            if (settings.mixed_nucleon_momentum) {
                // Alternate components: exactly half each for an even run, one extra p draw for an odd run.
                p = event.id % 2 == 0 ? random.Uniform(settings.nucleon_p_min, settings.nucleon_p_max) : 1.0 / random.Uniform(1.0 / settings.nucleon_p_max, 1.0 / settings.nucleon_p_min);
            }
            int pid = channel == UniformChannel::ElectronProton ? 2212 : 2112;
            event.particles.push_back({11, particleMass(11), momentum(beam, settings.trigger_theta, triggerPhi(phi, settings.trigger_phi_offset)), vertex});
            event.particles.push_back({pid, particleMass(pid), momentum(p, theta, phi), vertex});
        }
        writer.write(event);
        monitoring.fill(event);
        legacy_monitoring.fill(event);
    }
    monitoring.save(std::filesystem::path(c.get("output")) / "monitoring.root");
    legacy_monitoring.save(std::filesystem::path(c.get("output")) / "legacy_histograms.root", c.get("render-plots") == "true");
    writer.finish(writer.count());
    std::cout << "Wrote " << writer.count() << " events to " << c.get("output") << '\n';
}
}  // namespace samples
