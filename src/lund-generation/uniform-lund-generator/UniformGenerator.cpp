//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformGenerator.cpp
 * @brief Generates electron and electron-hadron acceptance samples.
 *
 * Purpose:
 *   Create the deliberately unphysical 1e and electron-hadron samples requested by generateUniform().
 *   Keep the established sampling rules while using the shared target, writer, monitoring, and naming code.
 *
 * Workflow:
 *   Check and convert settings -> start separate kinematic and vertex RNGs -> create particles and one
 *   shared vertex per event -> write split LUND files -> fill monitoring -> save ROOT/PDF/PNG plots ->
 *   finish the LUND files and publish the manifest.
 *
 * Reproducibility:
 *   Kinematics and target vertices use separate TRandom3 objects. Vertex sampling therefore cannot
 *   change the kinematic random sequence.
 */

#include "uniform-lund-generator/UniformGenerator.h"

#include <TMath.h>

#include <cmath>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"
#include "uniform-lund-generator/UniformConfig.h"
#include "uniform-lund-generator/UniformMonitoring.h"

namespace samples {

// Private kinematic helpers ---------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private kinematic helpers */
/**
 * @namespace samples::<anonymous>
 * @brief Small calculations used only by the uniform event loop.
 *
 * These helpers build names, convert coordinates, and choose the trigger sector. They stay private
 * because these rules belong only to uniform samples.
 */
namespace {

// sampleLabel -----------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* sampleLabel */
/**
 * @brief Build the monitoring label for one configured uniform sample.
 * @param config Resolved uniform configuration containing channel, hadron, and detector region.
 * @return `1e`, `electron-tester`, or the electron-hadron label with its FD/CD suffix.
 */
std::string sampleLabel(const RunConfig& c) {
    if (c.get("channel") == "1e") { return "1e"; }
    if (c.get("channel") == "electron-tester") { return "electron-tester"; }
    const std::string token = c.get("hadron") == "proton" ? "p" : c.get("hadron") == "neutron" ? "n" : c.get("hadron");
    return "e" + token + c.get("hadron-region");
}
#pragma endregion

// legacyBeamLabel -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* legacyBeamLabel */
/**
 * @brief Return the archived monitoring filename token for one beam energy.
 * @param beam Configured beam energy in GeV.
 * @return Legacy MeV label for established RG-M energies, or a rounded MeV label otherwise.
 */
std::string legacyBeamLabel(double beam) {
    if (std::abs(beam - 2.07052) < 1e-6) { return "2070MeV"; }
    if (std::abs(beam - 4.02962) < 1e-6) { return "4029MeV"; }
    if (std::abs(beam - 5.98636) < 1e-6) { return "5986MeV"; }
    if (std::abs(beam - 10.6) < 1e-6) { return "10600MeV"; }

    return std::to_string(static_cast<long long>(std::llround(beam * 1000))) + "MeV";
}
#pragma endregion

// momentum --------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* momentum */
/**
 * @brief Convert sampled polar coordinates to a momentum vector.
 *
 * Purpose:
 *   Convert configured degrees to the radians required by ROOT in one place.
 *
 * Algorithm:
 *   Create an empty TVector3, convert theta and phi to radians, and assign magnitude plus spherical
 *   direction through TVector3::SetMagThetaPhi().
 *
 * @param p Momentum magnitude in GeV/c. The configuration checks its allowed range.
 * @param theta Polar angle from the positive z axis, in degrees.
 * @param phi Azimuth about the z axis, in degrees.
 *
 * @return Cartesian momentum vector whose component unit is GeV/c.
 *
 * @note This function draws no random numbers and does not check acceptance ranges.
 */
TVector3 momentum(double p, double theta, double phi) {
    // ROOT expects angular inputs in radians even though profiles and monitoring use degrees.
    TVector3 v;
    v.SetMagThetaPhi(p, theta * TMath::DegToRad(), phi * TMath::DegToRad());

    return v;
}
#pragma endregion

// triggerPhi ------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* triggerPhi */
/**
 * @brief Choose the historical trigger-electron azimuth.
 *
 * Purpose:
 *   Place the artificial trigger electron near the CLAS12 sector closest to the direction opposite the
 *   sampled hadron instead of drawing a separate random azimuth.
 *
 * Algorithm:
 *   1. Add 180 degrees to the hadron azimuth and wrap once into the legacy [-180, 180] convention.
 *   2. Compare that opposite direction with sector centers spaced by 60 degrees.
 *   3. Retain the first center on an exact tie and add the configured beam-dependent offset.
 *
 * @param phi Sampled hadron azimuth in degrees, expected in [-180, 180].
 * @param offset Configured trigger-sector offset in degrees, already resolved and validated.
 *
 * @return Trigger-electron azimuth in degrees. The final offset result is intentionally not wrapped,
 *         matching the historical prescription consumed by TVector3.
 *
 * @note The opposite-sector relation is not obligatory for a CD hadron. It is retained deliberately
 *       to be sure the trigger electron follows the established separated placement.
 */
double triggerPhi(double phi, double offset) {
    // One full-turn correction is enough for the expected input range. Keep the ±180 endpoints.
    auto wrap = [](double angle) { return angle > 180 ? angle - 360 : angle < -180 ? angle + 360 : angle; };
    const double target = wrap(phi + 180);

    // Check sector centers in order. Strict `<` keeps the first center when two are equally close.
    double closest = -120, difference = std::abs(wrap(target - closest));

    for (double angle : {-60., 0., 60., 120., 180.}) {
        double d = std::abs(wrap(target - angle));

        if (d < difference) {
            difference = d;
            closest = angle;
        }
    }

    // Add the configured offset after choosing the sector. This draws no random value.
    return closest + offset;
}
#pragma endregion

}  // namespace
#pragma endregion

// generateUniform -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* generateUniform */
void generateUniform(const RunConfig& c) {
#pragma region /* Run preparation */
    // Check the settings again and print them before the writer replaces an existing run.
    c.validate(true);
    LundWriter::printWorkflowSummary(c, "uniform");

    // Convert strings to typed values once before the event loop.
    const UniformConfig settings(c);
    const auto channel = settings.channel;

    // Kinematics and vertices own separate streams. A nonzero seed makes its stream repeatable. ROOT
    // gives TRandom3(0) an automatically generated seed, which is a deliberate user-selectable choice
    // but prevents exact event replay from the recorded numeric setting alone. Stream separation keeps
    // target draws from shifting the electron/hadron sequence when both streams are reproducible.
    TRandom3 random(c.integer("seed")), vertex_random(c.integer("vertex-seed"));

    // Use the selected targets.h geometry for every event vertex.
    TargetGeometry geometry(c.get("target"));

    // The writer checks and replaces the exact run directory, creates its folders, and stores file limits.
    LundWriter writer(c, "uniform");

    // Monitoring keeps the established plots and adds FD/CD to hadron names.
    const double beam = settings.beam;
    UniformMonitoring monitoring(sampleLabel(c), settings.hadron_pid, beam);
#pragma endregion

#pragma region /* Event generation */
    // Create one complete event per loop. writer.count() changes only after a successful write.
    while (!writer.full()) {
        Event event;

        // The run-wide ID also chooses which half of mixed momentum sampling is used.
        event.id = writer.count();

        // A and Z are LUND header values and do not choose the vertex geometry.
        event.A = settings.A;
        event.Z = settings.Z;
        event.beam_energy = beam;

        // Sample exactly one target vertex with vertex_random and give it to every particle in the event.
        const auto vertex = geometry.sample(vertex_random);

        // The 1e branch writes one electron. Theta and phi retain the legacy flat-angle prescription.
        // Momentum may be uniform-p, an exactly alternating 50/50 uniform-p/uniform-1/p mixture, or
        // fixed to the beam value for the angular tester. Inverse-p sampling requires a positive minimum.
        if (channel != UniformChannel::ElectronHadron) {
            double theta = random.Uniform(settings.electron_theta_min, settings.electron_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_electron_momentum ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : beam;

            if (settings.mixed_electron_momentum) {
                p = event.id % 2 == 0 ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : 1.0 / random.Uniform(1.0 / settings.electron_p_max, 1.0 / settings.electron_p_min);
            }
            event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg), momentum(p, theta, phi), vertex});
        } else {
            // Acceptance-map coverage is flat in theta and phi inside the configured detector window.
            double theta = random.Uniform(settings.hadron_theta_min, settings.hadron_theta_max);

            // Azimuth is flat over the full signed range. The initial momentum covers fixed and
            // uniform-p modes; mixed mode deliberately replaces it below using the same stream.
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_hadron_momentum ? random.Uniform(settings.hadron_p_min, settings.hadron_p_max) : settings.hadron_p;

            if (settings.mixed_hadron_momentum) {
                // Even IDs draw p uniformly. Odd IDs draw 1/p uniformly between reciprocal bounds and
                // invert the result. An even-sized run is exactly 50/50; an odd run has one extra p draw.
                p = event.id % 2 == 0 ? random.Uniform(settings.hadron_p_min, settings.hadron_p_max) : 1.0 / random.Uniform(1.0 / settings.hadron_p_max, 1.0 / settings.hadron_p_min);
            }

            // Preserve stable particle order: the artificial beam-momentum trigger electron is first,
            // followed by the selected proton, neutron, pi+, or pi-. triggerPhi() correlates its azimuth with
            // the direction opposite the hadron and applies the configured sector offset.
            const int pid = settings.hadron_pid;
            event.particles.push_back(
                {constants::electron_pdg, particleMass(constants::electron_pdg), momentum(beam, settings.trigger_theta, triggerPhi(phi, settings.trigger_phi_offset)), vertex});
            event.particles.push_back({pid, particleMass(pid), momentum(p, theta, phi), vertex});
        }

        // Write first so monitoring never counts an event that failed to write.
        writer.write(event);
        monitoring.fill(event);
    }
#pragma endregion

#pragma region /* Run completion */
    // Save ROOT, PDF, and PNG monitoring before publishing the completion manifest.
    const auto output = std::filesystem::path(c.get("output"));
    const auto diagnostics = output / "lundfiles" / "lund-gen-monitoring";
    const auto monitoring_root = diagnostics / (c.get("prefix") + "_monitoring_plots.root");
    const auto plot_directory = diagnostics / "MonitoringPlotsPath";
    const auto plot_channel = sampleLabel(c);
    const auto pdf_name = "Uniform_" + plot_channel + "_plots_" + legacyBeamLabel(beam) + ".pdf";
    monitoring.save(monitoring_root, plot_directory, pdf_name);

    // Uniform generation creates and writes the same number of events. finish() closes the files and
    // publishes that count in the manifest.
    writer.finish(writer.count());

    // Print final counts only after the manifest is published. Both counts are equal for uniform runs.
    LundWriter::printWorkflowSummary(c, "uniform", writer.count(), writer.count(), true);
#pragma endregion
}
#pragma endregion

}  // namespace samples
