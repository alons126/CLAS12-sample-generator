//
// Created by Alon Sportes on 15/09/2026.
//

/**
 * @file UniformGenerator.cpp
 * @brief Electron and electron-hadron acceptance-sample generation.
 *
 * Purpose:
 *   Implement the deliberately unphysical 1e and configurable electron–hadron acceptance probes behind generateUniform(),
 *   preserving scientifically relevant legacy prescriptions while sharing maintained target geometry,
 *   LUND serialization, monitoring, naming, and provenance infrastructure.
 *
 * Workflow:
 *   Validate and cache settings -> initialize independent RNG streams and outputs -> sample one vertex
 *   and the selected particle content per event -> serialize successful events with the configured split
 *   size -> fill modern and legacy diagnostics -> save maintained and archived artifact layouts ->
 *   finalize LUND files and publish the manifest.
 *
 * Units and conventions:
 *   Momentum uses GeV/c and mass uses GeV/c², polar and azimuthal angles enter the sampler in degrees, and vertices
 *   use cm. Particle vectors follow ROOT's TVector3 spherical convention. Each event places its
 *   electron and optional hadron at one shared interaction vertex.
 *
 * Reproducibility:
 *   Kinematics and target vertices use separate configured TRandom3 streams. Draw order is part of the
 *   reproducibility contract, so target-geometry sampling cannot consume kinematic random numbers.
 */

#include "uniform/UniformGenerator.h"

#include <TMath.h>

#include <cmath>
#include <iostream>

#include "geometry/TargetGeometry.h"
#include "lund/LundWriter.h"
#include "monitoring/LegacyMonitoring.h"
#include "monitoring/Monitoring.h"
#include "support/constants.h"
#include "support/environment.h"
#include "uniform/UniformConfig.h"

namespace env = environment;

namespace samples {

// Private kinematic helpers ---------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Private kinematic helpers */
/**
 * @namespace samples::<anonymous>
 * @brief Translation-unit-only conversions used by the uniform event loop.
 *
 * These helpers contain coordinate and legacy trigger-sector mechanics. Keeping them private prevents
 * other workflows from depending on prescriptions that are specific to uniform acceptance samples.
 */
namespace {

std::string sampleLabel(const RunConfig& c) {
    if (c.get("channel") == "1e") { return "1e"; }
    const std::string token = c.get("hadron") == "proton" ? "p" : c.get("hadron") == "neutron" ? "n" : c.get("hadron");
    return "e" + token + c.get("hadron-region");
}

std::string compatibilityMonitoringChannel(const RunConfig& c) {
    if (c.get("channel") == "1e") { return c.get("electron-momentum") == "beam" ? "Tester_e" : "1e"; }
    if (c.get("hadron-region") == "FD" && c.get("hadron") == "proton") { return "ep"; }
    if (c.get("hadron-region") == "FD" && c.get("hadron") == "neutron") { return "en"; }
    return sampleLabel(c);
}

// legacyBeamLabel --------------------------------------------------------------------------------------------------------------------------------------------------------

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
 *   Centralize the degrees-to-radians boundary between configuration/sampling code and ROOT's vector
 *   API so every generated particle uses the same coordinate convention.
 *
 * Algorithm:
 *   Create an empty TVector3, convert theta and phi to radians, and assign magnitude plus spherical
 *   direction through TVector3::SetMagThetaPhi().
 *
 * @param p Momentum magnitude in GeV/c. RunConfig/UniformConfig validation owns range constraints.
 * @param theta Polar angle from the positive z axis, in degrees.
 * @param phi Azimuth about the z axis, in degrees.
 *
 * @return Cartesian momentum vector whose component unit is GeV/c.
 *
 * @note The helper consumes no random numbers and performs no wrapping or acceptance checks.
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
 *   Reproduce the legacy artificial trigger electron for two-particle samples, which lies near the CLAS12 sector closest
 *   to the direction opposite the sampled hadron rather than using an independent random azimuth.
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
    // Expected inputs require at most one full-turn correction; preserving ±180 endpoints maintains
    // the historical sector representation.
    auto wrap = [](double angle) { return angle > 180 ? angle - 360 : angle < -180 ? angle + 360 : angle; };
    const double target = wrap(phi + 180);

    // Seed with the first sector center, then visit remaining centers in legacy order. Strict `<`
    // prevents a later equidistant sector from replacing the earlier tie winner.
    double closest = -120, difference = std::abs(wrap(target - closest));

    for (double angle : {-60., 0., 60., 120., 180.}) {
        double d = std::abs(wrap(target - angle));

        if (d < difference) {
            difference = d;
            closest = angle;
        }
    }

    // Apply the beam-setting correction after sector selection, without consuming RNG state.
    return closest + offset;
}
#pragma endregion

}  // namespace
#pragma endregion

// generateUniform -------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* generateUniform */
/**
 * @brief Generate one completed uniform acceptance-sample run.
 *
 * Purpose:
 *   Produce a fixed-size acceptance probe with configured 1e or electron–hadron content while preserving the
 *   legacy channel prescriptions, deterministic random-stream separation, shared writer contract, and
 *   both modern and archived monitoring views.
 *
 * Workflow:
 *   1. Revalidate settings, print the resolved run, cache typed values, and initialize output objects.
 *   2. Create independent seeded RNG streams for particle kinematics and target vertices.
 *   3. Build one Event per iteration with configured A/Z and beam metadata plus one shared vertex.
 *   4. Sample the electron-only or artificial trigger-electron+hadron channel in stable draw order.
 *   5. Serialize the event before adding it to either diagnostic set.
 *   6. Stop at the writer's configured event capacity, save modern and archived-layout diagnostics,
 *      finalize LUND output, publish the manifest, and print the completion summary.
 *
 * @param c Borrowed configuration returned by RunConfig::parse(..., true). It supplies channel, beam
 *          energy in GeV, momenta in GeV/c, angles in degrees, vertices in cm, target/header metadata,
 *          RNG seeds, output formatting, monitoring, and the final run directory.
 *
 * @return Nothing. Normal return means all requested events and diagnostics were written and the
 *         completion manifest was published.
 *
 * @throws std::exception If validation, target sampling, guarded output replacement, serialization,
 *         monitoring, or manifest publication fails. No success manifest is published before all
 *         required output stages complete.
 *
 * @note This is an acceptance sampler, not a physical interaction model. electron–hadron trigger electrons are
 *       artificial. Production 1e and ep alternate uniform-p with uniform-1/p by run event ID, while
 *       production en samples p uniformly; explicit compatibility/study modes may override each rule.
 */
void generateUniform(const RunConfig& c) {
#pragma region /* Run preparation */
    // Recheck the public contract at the workflow boundary even when the caller used RunConfig::parse.
    // The initial summary exposes resolved settings before the writer replaces an existing run.
    c.validate(true);
    LundWriter::printWorkflowSummary(c, "uniform");

    // Convert repeated string lookups into immutable typed fields before entering the production loop.
    const UniformConfig settings(c);
    const auto channel = settings.channel;

    // Kinematics and vertices own separate streams. A nonzero seed makes its stream repeatable. ROOT
    // gives TRandom3(0) an automatically generated seed, which is a deliberate user-selectable choice
    // but prevents exact event replay from the recorded numeric setting alone. Stream separation keeps
    // target draws from shifting the electron/hadron sequence when both streams are reproducible.
    TRandom3 random(c.integer("seed")), vertex_random(c.integer("vertex-seed"));

    // Target mode delegates spatial draws to the external geometry implementation. Fixed mode selects
    // its valid point helper but bypasses sample() below and uses the configured coordinates directly.
    TargetGeometry geometry(c.get("vertex-mode") == "target" ? c.get("target") : "point");

    // Construction validates the final path, replaces an existing exact run, creates the legacy uniform
    // directory layout, and stores the requested total capacity and per-file split threshold.
    LundWriter writer(c, "uniform");

    // Monitoring ranges use the beam-energy scale. LegacyMonitoring selects its historical Tester_e
    // layout only for the electron channel whose momentum is fixed to the beam value.
    const double beam = settings.beam;
    Monitoring monitoring(beam);
    LegacyMonitoring legacy_monitoring(compatibilityMonitoringChannel(c), beam);
#pragma endregion

#pragma region /* Event generation */
    // Generate exactly one complete event per iteration until the writer reaches the configured total.
    // writer.count() advances only after successful LUND serialization.
    while (!writer.full()) {
        Event event;

        // The run-global ID also chooses the mixed ep component. In legacy output mode the serializer
        // may present a per-file ID without changing this internal run ordering.
        event.id = writer.count();

        // A/Z are LUND header metadata and do not select or alter the vertex geometry.
        event.A = settings.A;
        event.Z = settings.Z;
        event.beam_energy = beam;

        // Sample exactly one interaction vertex per event. Fixed mode consumes no vertex RNG draws;
        // target mode uses only vertex_random. Every particle below receives this same value.
        const auto vertex = c.get("vertex-mode") == "fixed" ? TVector3(c.number("vertex-x"), c.number("vertex-y"), c.number("vertex-z")) : geometry.sample(vertex_random);

        // The 1e branch writes one electron. Theta and phi retain the legacy flat-angle prescription.
        // Momentum may be uniform-p, an exactly alternating 50/50 uniform-p/uniform-1/p mixture, or
        // fixed to the beam value for the angular tester. Inverse-p sampling requires a positive minimum.
        if (channel == UniformChannel::Electron) {
            double theta = random.Uniform(settings.electron_theta_min, settings.electron_theta_max);
            double phi = random.Uniform(-180, 180);
            double p = settings.uniform_electron_momentum ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : beam;

            if (settings.mixed_electron_momentum) {
                p = event.id % 2 == 0 ? random.Uniform(settings.electron_p_min, settings.electron_p_max) : 1.0 / random.Uniform(1.0 / settings.electron_p_max, 1.0 / settings.electron_p_min);
            }
            event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg, settings.legacy_mass), momentum(p, theta, phi), vertex});
        } else {
            // The eh branch first samples the hadron direction. Isotropic mode draws cos(theta) uniformly only
            // inside the configured configured detector-region window; theta mode draws theta itself uniformly.
            double theta =
                settings.isotropic_hadron_angle
                    ? std::acos(random.Uniform(std::cos(settings.hadron_theta_max * TMath::DegToRad()), std::cos(settings.hadron_theta_min * TMath::DegToRad()))) * TMath::RadToDeg()
                    : random.Uniform(settings.hadron_theta_min, settings.hadron_theta_max);

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
            event.particles.push_back({constants::electron_pdg, particleMass(constants::electron_pdg, settings.legacy_mass),
                                       momentum(beam, settings.trigger_theta, triggerPhi(phi, settings.trigger_phi_offset)), vertex});
            event.particles.push_back({pid, particleMass(pid, settings.legacy_mass), momentum(p, theta, phi), vertex});
        }

        // Serialize first so neither monitoring file counts an event rejected by the writer. A later
        // monitoring failure leaves inspectable partial output but cannot publish a completion manifest.
        writer.write(event);
        monitoring.fill(event);
        legacy_monitoring.fill(event);
    }
#pragma endregion

#pragma region /* Run completion */
    // Persist both diagnostic contracts before making the run consumable. Optional rendering adds
    // legacy PDF/PNG views without changing histogram filling or LUND content.
    monitoring.save(std::filesystem::path(c.get("output")) / "monitoring.root");
    const auto output = std::filesystem::path(c.get("output"));
    const auto legacy_root = output / (c.get("prefix") + "_plots.root");
    const auto plot_directory = output / "MonitoringPlotsPath";
    const auto plot_channel = compatibilityMonitoringChannel(c);
    const auto pdf_name = "Uniform_" + plot_channel + "_plots_" + legacyBeamLabel(beam) + ".pdf";
    legacy_monitoring.save(legacy_root, c.get("render-plots") == "true", plot_directory, pdf_name);

    // Keep the maintained stable diagnostic name as an exact copy while making the archived prefix-based
    // filename the primary output artifact. Downstream validation can therefore retain one fixed path.
    std::filesystem::copy_file(legacy_root, output / "legacy_histograms.root", std::filesystem::copy_options::overwrite_existing);

    // Uniform generation scans and writes the same number of events, so the written count is also the
    // completion count supplied to the manifest. finish() closes files before publishing readiness.
    writer.finish(writer.count());

    // Print the legacy-compatible final summary only after successful manifest publication.
    LundWriter::printWorkflowSummary(c, "uniform", 0, writer.count(), true);
    std::cout << env::SYSTEM_COLOR << "Wrote " << writer.count() << " events to " << env::RESET_COLOR << c.get("output") << '\n';
#pragma endregion
}
#pragma endregion

}  // namespace samples
