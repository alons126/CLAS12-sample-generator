//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file uniform_lund_creator_main.cpp
 * @brief Starts uniform LUND creation from the command line.
 *
 * Purpose:
 *   Read command-line settings, run uniform generation, and return a process status.
 *
 * Workflow:
 *   Print help when requested -> read and check the options -> generate the sample -> return success or
 *   print a caught error.
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
 *   --help                           Print the complete runtime option summary.
 *
 * Output:
 *   A replaced run directory containing split LUND files, a settings record, and monitoring output.
 */

#include <exception>
#include <iostream>
#include <string>

#include "support/environment.h"
#include "uniform-lund-creator/UniformGenerator.h"

namespace env = environment;

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Run uniform generation from command-line arguments.
 *
 * Purpose:
 *   Handle help and errors here. RunConfig checks settings, and generateUniform() creates the sample.
 *
 * Workflow:
 *   Select uniform settings -> print help when requested -> check options -> generate LUND files ->
 *   return success or report an error.
 *
 * @param argc Number of argv entries, including the executable name.
 * @param argv Process arguments read during this call. This function does not change or store them.
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
    // Select the uniform branch of the shared parser.
    constexpr bool uniform = true;

    try {
        // Print help only when it is the sole user argument.
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << samples::help(uniform);
            return 0;
        }

        // Check every option before output can be replaced.
        samples::generateUniform(samples::RunConfig::parse(argc, argv, uniform));

        // Generation finished successfully.
        return 0;
    } catch (const std::exception& error) {
        // Reset the color before the error text and return failure.
        std::cerr << env::ERROR_COLOR << "Error: " << env::RESET_COLOR << error.what() << '\n';

        return 1;
    }
}
#pragma endregion
