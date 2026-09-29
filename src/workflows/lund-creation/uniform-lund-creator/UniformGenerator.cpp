/**
 * @file UniformGenerator.cpp
 * @brief Creates random electron and electron-hadron LUND events for detector tests.
 *
 * Purpose:
 *   Cover configured particle momentum and angle ranges so later detector simulation can show which
 *   particles CLAS12 detects. These events are deliberately random and do not represent a physical
 *   interaction. The code uses the shared vertex-position, LUND-writing, progress, and monitoring tools.
 *
 * Execution flow:
 *   Check and print the settings. Start separate random-number generators for particle motion and target
 *   positions. Prepare the run directory and monitoring plots. Create each event, write it to LUND, and
 *   add its values to the plots. Finally, save the plots and publish the completion manifest. Stage
 *   messages and a progress bar show what work is happening.
 *
 * Repeatability:
 *   Particle motion and vertex positions use separate ROOT TRandom3 objects. Choosing a vertex position
 *   therefore does not consume a random value from the sequence used for momentum and angles. A nonzero
 *   seed repeats the same sequence. ROOT treats seed 0 as a request to choose a new seed, so that sequence
 *   cannot be repeated from the recorded zero alone.
 */

#include "uniform-lund-creator/UniformGenerator.h"

#include <TMath.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#include "core/geometry/TargetGeometry.h"
#include "core/lund/LundWriter.h"
#include "core/presentation/ProgressReporter.h"
#include "support/environment.h"
#include "uniform-lund-creator/UniformConfig.h"
#include "uniform-lund-creator/UniformMonitoring.h"

namespace env = environment;

namespace samples {

// Private event-building helpers ----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private event-building helpers */
/**
 * @namespace samples::<anonymous>
 * @brief Contains small naming and direction calculations used only in this source file.
 *
 * These helpers build the monitoring label, turn momentum size and angles into a three-dimensional vector,
 * and place the trigger electron near the detector sector opposite the hadron.
 */
namespace {

// Building the monitoring label -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Building the monitoring label */
/**
 * @brief Build the short sample name used in monitoring plots.
 * @param c Final settings containing the channel, hadron, and detector region.
 * @return `1e`, `electron-tester`, or a label such as `epFD` for an electron, proton, and forward
 *         detector.
 */
std::string sampleLabel(const RunConfig& c) {
    if (c.getText("channel") == "1e") { return "1e"; }
    if (c.getText("channel") == "electron-tester") { return "electron-tester"; }

    const std::string token = (c.getText("hadron") == "proton") ? "p" : (c.getText("hadron") == "neutron") ? "n" : c.getText("hadron");

    return "e" + token + c.getText("hadron-region");
}
#pragma endregion

// Building a momentum vector --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Building a momentum vector */
/**
 * @brief Turn momentum size and two direction angles into x, y, and z momentum.
 *
 * Purpose:
 *   Settings and monitoring use degrees, but ROOT's TVector3 function expects radians. Keep that conversion
 *   in one place.
 *
 * Conversion:
 *   Theta measures the angle away from the positive z-axis, which is the beam direction. Phi measures the
 *   angle around that axis. Convert both angles to radians, then let ROOT calculate the three components.
 *
 * @param p Size of the momentum in GeV/c.
 * @param theta Angle away from the positive z-axis, in degrees.
 * @param phi Angle around the z-axis, in degrees.
 * @return Three-dimensional momentum whose x, y, and z components are measured in GeV/c.
 * @note RunConfig has already checked the allowed ranges. This function does not draw random numbers.
 */
TVector3 momentum(double p, double theta, double phi) {
    // Convert the two angles from degrees to the radians expected by ROOT.
    TVector3 v;
    v.SetMagThetaPhi(p, theta * TMath::DegToRad(), phi * TMath::DegToRad());

    return v;
}
#pragma endregion

// Choosing the trigger-electron direction -------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Choosing the trigger-electron direction */
/**
 * @brief Choose the trigger electron's phi near the detector sector opposite the hadron.
 *
 * Purpose:
 *   CLAS12's forward detector is divided into six 60-degree sectors. Electron-hadron test events place the
 *   electron near the sector facing away from the hadron. This gives the two particles clear separation.
 *
 * Selection:
 *   Add 180 degrees to find the direction opposite the hadron. Choose the nearest detector-sector center,
 *   then add the configured offset. If two centers are equally close, keep the first one checked.
 *
 * @param phi Hadron angle around the beam direction, in degrees from -180 through 180.
 * @param offset Angle added after choosing the sector center, in degrees.
 * @return Trigger-electron phi in degrees. The result may fall outside -180 through 180 after the offset
 *         is added.
 * @note A hadron in the central detector does not require this separation, but using the same rule keeps a
 *       clear separation check in those samples too.
 */
double triggerPhi(double phi, double offset) {
    // Move an angle that crossed one end of the -180-to-180 range back by one full turn. The allowed input
    // can cross an end only once.
    auto wrap = [](double angle) { return (angle > 180) ? angle - 360 : (angle < -180) ? angle + 360 : angle; };
    const double target = wrap(phi + 180);

    // Check the six sector centers in order. Strict `<` keeps the first center when two are equally close.
    double closest = -120, difference = std::abs(wrap(target - closest));

    for (double angle : {-60., 0., 60., 120., 180.}) {
        double d = std::abs(wrap(target - angle));

        if (d < difference) {
            difference = d;
            closest = angle;
        }
    }

    // Add the configured offset after choosing the nearest sector center. This calculation uses no random
    // number.
    return closest + offset;
}
#pragma endregion

}  // namespace
#pragma endregion

// Creating the uniform LUND run -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Creating the uniform LUND run */
void generateUniform(const RunConfig& c) {
#pragma region /* Run preparation */
    std::cout << "\n" << env::SYSTEM_COLOR << "Validating uniform settings and preparing LUND output..." << env::RESET_COLOR << "\n";

    // Check and show every final setting before LundWriter may replace an existing run directory.
    c.validateForSource(LundSource::Uniform);
    LundWriter::printWorkflowSummary(c, "uniform");

    // Convert text settings to numbers and named choices once instead of repeating conversions for every
    // event.
    const UniformConfig settings(c);
    const auto channel = settings.channel;

    // Use separate random-number generators for particle motion and vertex positions. A nonzero seed
    // repeats its sequence. ROOT chooses a new seed for TRandom3(0), so a recorded zero cannot repeat that
    // run later.
    TRandom3 random(c.getNonnegativeInteger("seed")), vertex_random(c.getNonnegativeInteger("vertex-seed"));

    // Use the selected targets.h shape and position rules for every event.
    TargetGeometry geometry(c.getText("target-geometry"));

    // LundWriter safely replaces the exact run directory, creates its folders, and stores the event and
    // per-file limits.
    LundWriter writer(c, "uniform");

    // Create the plot set for this sample. Hadron plot names include FD or CD.
    const double beam = settings.beam;
    UniformMonitoring monitoring(sampleLabel(c), settings.hadron_pid, beam);
#pragma endregion

#pragma region /* Event generation */
    std::cout << "\n" << env::SYSTEM_COLOR << "Generating uniform events and writing LUND output..." << env::RESET_COLOR << "\n";

    ProgressReporter progress("Generating LUND events", static_cast<std::uint64_t>(c.getNonnegativeInteger("events")));
    progress.update(0);

    // Create one event in each loop. getWrittenEventCount() increases only after the complete event is
    // written.
    while (!writer.hasReachedRunEventLimit()) {
        Event event;

        // Use the number of completed events as the next zero-based event ID. In mixed mode, this even or
        // odd ID also chooses between uniform-p and uniform-1/p sampling.
        event.id = writer.getWrittenEventCount();

        // In the LUND header, A is the total number of protons and neutrons and Z is the number of protons.
        // These values do not choose the target shape or vertex position.
        event.A = settings.A;
        event.Z = settings.Z;
        event.beam_energy = beam;

        // Choose exactly one vertex position with vertex_random. Give those same Vx, Vy, and Vz
        // coordinates to every particle in this event.
        const auto vertex = geometry.sampleVertexPosition(vertex_random);

        // For an electron-only event, choose theta and phi with equal probability across their ranges.
        // Momentum is uniform-p, alternating uniform-p/uniform-1/p, or fixed at beam momentum for the
        // electron test.
        if (channel != UniformChannel::ElectronHadron) {
            double theta = random.Uniform(settings.electron_theta_min, settings.electron_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_electron_momentum ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : beam;

            if (settings.mixed_electron_momentum) {
                p = (event.id % 2 == 0) ? random.Uniform(settings.electron_p_min, settings.electron_p_max)
                                        : 1.0 / random.Uniform(1.0 / settings.electron_p_max, 1.0 / settings.electron_p_min);
            }

            event.particles.push_back({constants::electron_pdg, getParticleMass(constants::electron_pdg), momentum(p, theta, phi), vertex});
        } else {
            // Choose the hadron's theta with equal probability across the configured detector range.
            double theta = random.Uniform(settings.hadron_theta_min, settings.hadron_theta_max);

            // Choose hadron phi across the full circle. First choose uniform or fixed momentum; mixed mode
            // replaces that value below.
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_hadron_momentum ? random.Uniform(settings.hadron_p_min, settings.hadron_p_max) : settings.hadron_p;

            if (settings.mixed_hadron_momentum) {
                // Even IDs choose p uniformly between its limits. Odd IDs choose 1/p uniformly and then
                // invert it. An even event count gives an exact half-and-half split. An odd count has one
                // extra uniform-p event because ID 0 is even.
                p = (event.id % 2 == 0) ? random.Uniform(settings.hadron_p_min, settings.hadron_p_max) : 1.0 / random.Uniform(1.0 / settings.hadron_p_max, 1.0 / settings.hadron_p_min);
            }

            // Keep the required particle order: trigger electron first and selected hadron second. Place
            // the electron near the detector sector opposite the hadron.
            const int pid = settings.hadron_pid;
            event.particles.push_back(
                {constants::electron_pdg, getParticleMass(constants::electron_pdg), momentum(beam, settings.trigger_theta, triggerPhi(phi, settings.trigger_phi_offset)), vertex});
            event.particles.push_back({pid, getParticleMass(pid), momentum(p, theta, phi), vertex});
        }

        // Write first so the monitoring plots never count an event that failed to reach the LUND file.
        writer.writeEvent(event);
        monitoring.addEventToHistograms(event);
        progress.update(writer.getWrittenEventCount());
    }

    progress.finish(writer.getWrittenEventCount(), 0, 0, "requested event count reached");
#pragma endregion

#pragma region /* Run completion */
    std::cout << "\n" << env::SYSTEM_COLOR << "Saving monitoring plots and finalizing LUND output..." << env::RESET_COLOR << "\n";

    // Save the ROOT histogram file, multipage PDF, and PNG images before publishing the manifest that marks
    // the run complete.
    const auto output = std::filesystem::path(c.getText("output"));
    const auto diagnostics = output / "lundfiles" / "lund-creation-monitoring";
    const auto monitoring_root = diagnostics / (c.getText("prefix") + "__monitoring_plots.root");
    const auto plot_directory = diagnostics / "MonitoringPlotsPath";
    const auto pdf_name = c.getText("prefix") + "__plots.pdf";
    monitoring.saveHistogramsAndRenderPlots(monitoring_root, plot_directory, pdf_name);

    // The uniform LUND creator writes every event it creates, so scanned and written counts are equal.
    // finalizeRun() closes the LUND file and records that count in the manifest.
    writer.finalizeRun(writer.getWrittenEventCount());

    // Print final counts only after the completed-run manifest has been published.
    LundWriter::printWorkflowSummary(c, "uniform", writer.getWrittenEventCount(), writer.getWrittenEventCount(), true);
#pragma endregion
}
#pragma endregion

}  // namespace samples
