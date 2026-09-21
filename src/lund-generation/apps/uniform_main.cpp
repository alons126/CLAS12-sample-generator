//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file uniform_main.cpp
 * @brief Uniform-generator command-line entry point.
 *
 * Purpose:
 *   Translate CLI settings into one generation call and a process exit status.
 *
 * Workflow:
 *   Help returns immediately; otherwise parse -> generateUniform -> report success or caught failure.
 *
 * CLI options:
 *   --config FILE                    Read `key = value` settings; CLI values take precedence.
 *   --channel 1e|eh|electron-tester  Select the generated final state (default: 1e).
 *   --hadron proton|neutron|pip|pim  Select the hadron when `--channel eh` (default: proton).
 *   --hadron-region FD|CD            Select the hadron detector region (default: FD).
 *   --beam-energy GeV                Set beam energy (default: 5.98636 GeV).
 *   --rgm-target ID                  Select nuclear metadata and automatic geometry (default: Ar40).
 *   --target GEOMETRY                Override the target-geometry key (default: auto).
 *   --A N / --Z N                    Override LUND target metadata (default: auto).
 *   --output DIRECTORY               Required parent directory for the resolved run directory.
 *   --events N                       Required total generated-event count.
 *   --events-per-file N              Split output after N events (default: 25000).
 *   --seed N / --vertex-seed N       Set kinematic/vertex ROOT seeds (defaults: 67890/12345; 0 is automatic).
 *   --prefix NAME                    Override the automatic LUND filename prefix.
 *   --electron-theta-min/max DEG     Set electron polar-angle bounds (defaults: 5/40 degrees).
 *   --electron-p-min/max GeV/c       Set electron momentum bounds (defaults: 0.7/beam momentum).
 *   --electron-momentum MODE         Select auto, uniform, mixed, or beam momentum.
 *   --hadron-theta-min/max DEG       Override region/species angle bounds (defaults: auto).
 *   --hadron-p-min GeV/c             Override the species/region momentum minimum (default: auto).
 *   --hadron-p GeV/c                 Set fixed neutron momentum (default: 1 GeV/c).
 *   --hadron-momentum MODE           Select auto, fixed, sampled, uniform, or mixed momentum.
 *   --trigger-theta DEG              Set the electron trigger angle for eh (default: 25 degrees).
 *   --trigger-phi-offset DEG         Override the electron/hadron azimuthal separation (default: auto).
 *   --help                           Print the authoritative runtime option summary.
 *
 * Output:
 *   A replaced run directory containing split LUND files, provenance, and uniform monitoring output.
 */

#include <exception>
#include <iostream>
#include <string>

#include "clas12-uniform/UniformGenerator.h"
#include "core/support/environment.h"

namespace env = environment;

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Uniform-generator command-line entry point.
 *
 * Purpose:
 *   Keep process-level concerns at the application boundary: select uniform-mode configuration,
 *   handle the standalone help request, invoke the maintained generator, and translate exceptions
 *   into a stable command-line failure status.
 *
 * Workflow:
 *   1. Mark this executable as the uniform source for shared help and configuration parsing.
 *   2. Print help and stop when `--help` is the only user argument.
 *   3. Parse and validate all generation options into a RunConfig.
 *   4. Generate the configured LUND files.
 *   5. Return success, or report a caught standard exception and return failure.
 *
 * @param argc Number of argv entries, including the executable name.
 * @param argv Borrowed process argument array. The parser reads its strings during this call; this
 *             function neither owns nor modifies their storage.
 *
 * @return `0` after help or successful generation; `1` when parsing or generation throws a
 *         `std::exception`.
 *
 * @note This executable creates deliberately unphysical uniform acceptance samples. It does not
 *       convert physical event-generator input or submit GEMC jobs.
 *
 * @throws Nothing for standard parser/generator failures: they are caught here and rendered to
 *         standard error. Non-standard exceptions are outside this boundary.
 */
int main(int argc, char** argv) {
    // Shared CLI/configuration helpers serve both uniform and physical applications. This immutable
    // mode selector chooses uniform help text and uniform-only validation for this entire process.
    constexpr bool uniform = true;

    try {
        // Treat only the exact standalone request as launcher help. All other argument combinations,
        // including `--help` mixed with run options, go through RunConfig::parse for one consistent
        // syntax and validation decision.
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << samples::help(uniform);
            return 0;
        }

        // RunConfig::parse owns option validation and returns the complete value object consumed by
        // generateUniform. The generator then owns output-directory replacement, event production,
        // LUND serialization, monitoring, provenance, and completion reporting for this run.
        samples::generateUniform(samples::RunConfig::parse(argc, argv, uniform));

        // Reaching this point means configuration and the complete generation workflow succeeded.
        return 0;
    } catch (const std::exception& error) {
        // Keep the prefix visually distinct while resetting color before the exception detail. A
        // single stable status lets workflow.py stop subsequent stages and report command failure.
        std::cerr << env::ERROR_COLOR << "Error: " << env::RESET_COLOR << error.what() << '\n';

        return 1;
    }
}
#pragma endregion
