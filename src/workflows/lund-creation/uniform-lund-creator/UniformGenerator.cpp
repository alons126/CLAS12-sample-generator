//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformGenerator.cpp
 * @brief Generates electron and electron-hadron acceptance samples.
 *
 * Purpose:
 *   Create the unphysical 1e and electron-hadron samples requested by generateUniform(). The code uses
 *   the shared target, writer, monitoring, and naming tools.
 *
 * Workflow:
 *   Check the settings -> start separate random-number generators for motion and vertices -> create each
 *   event -> write LUND files -> fill and save the plots -> close the files and write the run log.
 *
 * Repeatability:
 *   Particle motion and target vertices use separate TRandom3 objects. Drawing a vertex therefore does
 *   not change the random numbers used for particle motion.
 */

#include "uniform-lund-creator/UniformGenerator.h"

#include <TMath.h>

#include <cmath>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"
#include "uniform-lund-creator/UniformConfig.h"
#include "uniform-lund-creator/UniformMonitoring.h"

namespace samples {

// Private kinematic helpers ---------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private kinematic helpers */
/**
 * @namespace samples::<anonymous>
 * @brief Small calculations used only by the uniform event loop.
 *
 * These helpers build names, convert coordinates, and choose the trigger sector. Only this file uses them.
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
 * @brief Return the established monitoring filename text for one beam energy.
 * @param beam Configured beam energy in GeV.
 * @return The usual MeV label for known RG-M energies, or a rounded MeV label otherwise.
 */
std::string legacyBeamLabel(double beam) {
    if (std::abs(beam - 2.07052) < 1e-6) { return "2070MeV"; }
    if (std::abs(beam - 4.02962) < 1e-6) { return "4029MeV"; }
    if (std::abs(beam - 5.98636) < 1e-6) { return "5986MeV"; }

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
 * Steps:
 *   Convert theta and phi to radians, then give ROOT the momentum size and direction.
 *
 * @param p Momentum magnitude in GeV/c. The configuration checks its allowed range.
 * @param theta Polar angle from the positive z axis, in degrees.
 * @param phi Azimuth about the z axis, in degrees.
 *
 * @return Cartesian momentum vector whose component unit is GeV/c.
 *
 * @note This function does not draw random numbers or check the allowed ranges.
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
 * @brief Choose the established trigger-electron azimuth.
 *
 * Purpose:
 *   Place the trigger electron near the CLAS12 sector opposite the sampled hadron.
 *
 * Steps:
 *   Find the direction opposite the hadron, choose the nearest 60-degree sector center, and add the
 *   configured offset. An exact tie uses the first center checked.
 *
 * @param phi Sampled hadron azimuth in degrees, expected in [-180, 180].
 * @param offset Configured trigger-sector offset in degrees, already resolved and validated.
 *
 * @return Trigger-electron azimuth in degrees. The result is not wrapped after adding the offset.
 *
 * @note A CD hadron does not require this separation, but the same rule is kept for a clear check.
 */
double triggerPhi(double phi, double offset) {
    // One full-turn correction covers the allowed input range. Keep both 180-degree endpoints.
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

    // Add the offset after choosing the sector. No random number is used here.
    return closest + offset;
}
#pragma endregion

}  // namespace
#pragma endregion

// generateUniform -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* generateUniform */
void generateUniform(const RunConfig& c) {
#pragma region /* Run preparation */
    std::cout << "\n" << env::SYSTEM_COLOR << "Initializing..." << env::RESET_COLOR << "\n";
    
    // Check and print the settings before the writer replaces an existing run.
    c.validate(true);
    LundWriter::printWorkflowSummary(c, "uniform");

    // Convert strings to typed values once before the event loop.
    const UniformConfig settings(c);
    const auto channel = settings.channel;

    // Motion and vertices use separate random streams. A nonzero seed can be repeated. ROOT gives
    // TRandom3(0) a new automatic seed, so a run with seed zero cannot be replayed from that value alone.
    TRandom3 random(c.integer("seed")), vertex_random(c.integer("vertex-seed"));

    // Use the selected targets.h geometry for every event vertex.
    TargetGeometry geometry(c.get("target"));

    // The writer safely replaces the chosen run directory, creates it, and stores the file limits.
    LundWriter writer(c, "uniform");

    // Monitoring keeps the established plots and adds FD/CD to hadron names.
    const double beam = settings.beam;
    UniformMonitoring monitoring(sampleLabel(c), settings.hadron_pid, beam);
#pragma endregion

#pragma region /* Event generation */
    std::cout << "\n" << env::SYSTEM_COLOR << "Creating samples..." << env::RESET_COLOR << "\n";
    
    // Create one event per loop. writer.count() changes only after a successful write.
    while (!writer.full()) {
        Event event;

        // The event ID also selects uniform-p or uniform-1/p sampling in mixed mode.
        event.id = writer.count();

        // A and Z are LUND header values and do not choose the vertex geometry.
        event.A = settings.A;
        event.Z = settings.Z;
        event.beam_energy = beam;

        // Sample exactly one target vertex with vertex_random and give it to every particle in the event.
        const auto vertex = geometry.sample(vertex_random);

        // Electron-only events use flat theta and phi ranges. Momentum is uniform-p, alternating
        // uniform-p/uniform-1/p, or fixed at the beam value for the angular tester.
        if (channel != UniformChannel::ElectronHadron) {
            double theta = random.Uniform(settings.electron_theta_min, settings.electron_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_electron_momentum ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : beam;

            if (settings.mixed_electron_momentum) {
                p = event.id % 2 == 0 ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : 1.0 / random.Uniform(1.0 / settings.electron_p_max, 1.0 / settings.electron_p_min);
            }
            event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg), momentum(p, theta, phi), vertex});
        } else {
            // Draw theta and phi evenly across the configured detector window.
            double theta = random.Uniform(settings.hadron_theta_min, settings.hadron_theta_max);

            // Draw phi across the full range. Mixed mode replaces the first momentum value below.
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_hadron_momentum ? random.Uniform(settings.hadron_p_min, settings.hadron_p_max) : settings.hadron_p;

            if (settings.mixed_hadron_momentum) {
                // Even IDs draw p. Odd IDs draw 1/p and invert it. This gives an exact 50/50 split for
                // an even number of events and one extra p event for an odd number.
                p = event.id % 2 == 0 ? random.Uniform(settings.hadron_p_min, settings.hadron_p_max) : 1.0 / random.Uniform(1.0 / settings.hadron_p_max, 1.0 / settings.hadron_p_min);
            }

            // Keep the trigger electron first and the selected hadron second. Place the electron near
            // the sector opposite the hadron.
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
    std::cout << "\n" << env::SYSTEM_COLOR << "Finishing..." << env::RESET_COLOR << "\n";
    
    // Save the ROOT, PDF, and PNG plots before writing the completed run log.
    const auto output = std::filesystem::path(c.get("output"));
    const auto diagnostics = output / "lundfiles" / "lund-creation-monitoring";
    const auto monitoring_root = diagnostics / (c.get("prefix") + "_monitoring_plots.root");
    const auto plot_directory = diagnostics / "MonitoringPlotsPath";
    const auto plot_channel = sampleLabel(c);
    const auto pdf_name = "Uniform_" + plot_channel + "_plots_" + legacyBeamLabel(beam) + ".pdf";
    monitoring.save(monitoring_root, plot_directory, pdf_name);

    // Uniform generation creates and writes the same number of events. finish() closes the files and
    // records that count in the run log.
    writer.finish(writer.count());

    // Print final counts after the run log is complete. Both counts are equal for uniform runs.
    LundWriter::printWorkflowSummary(c, "uniform", writer.count(), writer.count(), true);
#pragma endregion
}
#pragma endregion

}  // namespace samples
