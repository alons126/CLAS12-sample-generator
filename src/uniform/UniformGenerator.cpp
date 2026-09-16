//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file UniformGenerator.cpp
 * @brief Electron and electron-nucleon acceptance-sample generation.
 *
 * Purpose:
 *   Generate configured momentum and angular distributions with legacy-compatible fixed modes.
 *
 * Workflow:
 *   Validate -> initialize RNGs and outputs -> sample vertex and particles -> fill diagnostics -> finish.
 */

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
// momentum ----------------------------------------------------------------------

#pragma region /* momentum */
/**
 * @brief Convert sampled polar coordinates to a momentum vector.
 *
 * Algorithm:
 *   Convert degrees to radians and set vector magnitude, theta and phi.
 *
 * @param p Momentum magnitude in GeV.
 * @param theta Polar angle in degrees.
 * @param phi Azimuthal angle in degrees.
 *
 * @return Cartesian momentum vector in GeV.
 */
TVector3 momentum(double p, double theta, double phi) {
    TVector3 v;
    v.SetMagThetaPhi(p, theta * TMath::DegToRad(), phi * TMath::DegToRad());

    return v;
}
#pragma endregion

// triggerPhi ----------------------------------------------------------------------

#pragma region /* triggerPhi */
/**
 * @brief Choose the historical trigger-electron azimuth.
 *
 * Algorithm:
 *   Wrap the opposite nucleon direction, find the nearest sector center, then add the beam-dependent offset.
 *
 * @param phi Nucleon azimuth in degrees.
 * @param offset Configured trigger offset in degrees.
 *
 * @return Trigger azimuth in degrees, retaining the legacy sector tie convention.
 */
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
#pragma endregion

}  // namespace
// generateUniform ----------------------------------------------------------------------

#pragma region /* generateUniform */
/**
 * @brief Generate one completed uniform acceptance-sample run.
 *
 * Purpose:
 *   Produce acceptance probes with configured distributions and reproducible independent vertex sampling.
 *
 * Algorithm:
 *   1. Validate and cache settings; initialize separate kinematic and vertex RNGs.
 *   2. Sample a common vertex and the selected electron or electron-nucleon channel.
 *   3. Write events and fill both diagnostic sets until capacity is reached.
 *   4. Save diagnostics and publish the manifest.
 *
 * @param c Resolved channel, beam, target, sampling, seed and output settings.
 *
 * @note No value; throws on invalid settings or output failure. Sampled ep alternates uniform-p and uniform-1/p draws.
 */
void generateUniform(const RunConfig& c) {
#pragma region /* Run preparation */
    // Prepare validated settings and independent random streams for this run.
    c.validate(true);
    LundWriter::printWorkflowSummary(c, "uniform");
    const UniformConfig settings(c);
    const auto channel = settings.channel;
    TRandom3 random(c.integer("seed")), vertex_random(c.integer("vertex-seed"));
    TargetGeometry geometry(c.get("vertex-mode") == "target" ? c.get("target") : "point");
    LundWriter writer(c, "uniform");
    const double beam = settings.beam;
    Monitoring monitoring(beam);
    LegacyMonitoring legacy_monitoring(c.get("channel") == "1e" && c.get("electron-momentum") == "beam" ? "Tester_e" : c.get("channel"), beam);
#pragma endregion

#pragma region /* Event generation */
    // Generate and write one complete event per iteration.
    while (!writer.full()) {
        Event event;
        event.id = writer.count();
        event.A = settings.A;
        event.Z = settings.Z;
        event.beam_energy = beam;

        // All particles in one event share the sampled interaction vertex.
        const auto vertex = c.get("vertex-mode") == "fixed" ? TVector3(c.number("vertex-x"), c.number("vertex-y"), c.number("vertex-z")) : geometry.sample(vertex_random);

        // Choose the electron-only or artificial trigger-electron plus nucleon prescription.
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

        // Monitor only events that were successfully serialized.
        writer.write(event);
        monitoring.fill(event);
        legacy_monitoring.fill(event);
    }
#pragma endregion

    // Persist diagnostics before making the run consumable through its manifest.
    monitoring.save(std::filesystem::path(c.get("output")) / "monitoring.root");
    legacy_monitoring.save(std::filesystem::path(c.get("output")) / "legacy_histograms.root", c.get("render-plots") == "true");
    writer.finish(writer.count());
    LundWriter::printWorkflowSummary(c, "uniform", 0, writer.count(), true);
    std::cout << "\033[33mWrote " << writer.count() << " events to \033[0m" << c.get("output") << '\n';
}
#pragma endregion

}  // namespace samples
