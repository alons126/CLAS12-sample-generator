//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file uniform_lund_creator_main.cpp
 * @brief Reads command-line settings and starts the uniform LUND creator.
 *
 * Purpose:
 *   This file is the installed `uniform-lund-creator` program. It handles `--help`, turns the supplied
 *   options into a checked RunConfig, runs uniform LUND creation, and tells the calling shell whether the
 *   operation succeeded.
 *
 * Workflow:
 *   If `--help` is the only option, print the help text and stop successfully. Otherwise, read and check
 *   every option before any output directory can be replaced. Run the uniform LUND creator. Return success
 *   when it finishes, or print one standard error and return failure when an exception is reported.
 *
 * CLI options:
 *   --config FILE                       Read `key = value` settings (default: no configuration file).
 *   --channel 1e|eh|electron-tester     Select the created final state (default: 1e).
 *   --hadron proton|neutron|pip|pim     Select the hadron for `eh` (default: proton).
 *   --hadron-region FD|CD               Select the hadron detector region (default: FD).
 *   --beam-energy GeV                   Set the incident-electron energy (default: 5.98636 GeV).
 *   --target ID                         Select the target material (default: Ar40).
 *   --gemc-target-variation NAME        Select a compatible GEMC target setup (default: auto).
 *   --A N                               Set the LUND target A value (default: selected target's A).
 *   --Z N                               Set the LUND target Z value (default: selected target's Z).
 *   --output DIRECTORY                  Select the parent output directory (required).
 *   --events N                          Set the number of events to create (required).
 *   --events-per-file N                 Split output after N events (default: 25000).
 *   --seed N                            Set the particle-motion seed (default: 67890; 0 asks ROOT for a new seed).
 *   --vertex-seed N                     Set the target-position seed (default: 12345; 0 asks ROOT for a new seed).
 *   --prefix NAME                       Set the LUND filename prefix (default: auto from run settings).
 *   --electron-theta-min DEG            Set the electron theta minimum (default: 5 degrees).
 *   --electron-theta-max DEG            Set the electron theta maximum (default: 40 degrees).
 *   --electron-p-min GeV/c              Set the electron momentum minimum (default: 0.7 GeV/c).
 *   --electron-p-max GeV/c              Set the electron momentum maximum (default: beam momentum).
 *   --electron-momentum MODE            Select auto, uniform, mixed, or beam momentum (default: auto by channel).
 *   --hadron-theta-min DEG              Set the hadron theta minimum (default: auto by particle and region).
 *   --hadron-theta-max DEG              Set the hadron theta maximum (default: auto by particle and region).
 *   --hadron-p-min GeV/c                Set the hadron momentum minimum (default: auto by particle and region).
 *   --hadron-p GeV/c                    Set the fixed neutron momentum (default: 1 GeV/c).
 *   --hadron-momentum MODE              Select auto, fixed, sampled, uniform, or mixed momentum (default: auto; sampled means auto).
 *   --trigger-theta DEG                 Set the trigger-electron theta (default: 25 degrees).
 *   --trigger-phi-offset DEG            Set its opposite-sector offset (default: auto by beam energy).
 *   --help                              Print this complete option summary when used alone.
 *
 * Output:
 *   The program replaces the fully checked final run directory and writes numbered LUND files, monitoring
 *   plots, empty directories for later simulation output, and a completion manifest. Event IDs begin at
 *   zero and continue across LUND file boundaries.
 */

#include <exception>
#include <iostream>
#include <string>

#include "support/environment.h"
#include "uniform-lund-creator/UniformGenerator.h"

namespace env = environment;

// Command-line entry point ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Command-line entry point */
/**
 * @brief Run the uniform LUND creator from command-line arguments.
 *
 * Purpose:
 *   Keep command-line help, process return values, and the final error message at the program boundary.
 *   RunConfig prepares the settings, and generateUniform() creates the complete sample.
 *
 * Workflow:
 *   Select the uniform LUND source. Print help when requested. Otherwise, create a checked RunConfig and
 *   pass it to generateUniform(). Return 0 after success. If either stage reports an error, print it with
 *   the standard project prefix and return 1.
 *
 * @param argc Number of command-line strings, including the executable name.
 * @param argv Command-line strings read during this call. main() does not change or retain them.
 *
 * @return 0 after printing help or completing the run; 1 when setting preparation or LUND creation reports
 *         a standard C++ exception.
 *
 * @note This program creates deliberately random detector-test samples. It does not convert physical
 *       event-generator input or submit GEMC simulation jobs.
 *
 * @note Exceptions that do not derive from std::exception are not caught and may leave this function.
 */
int main(int argc, char** argv) {
    // Select the settings and help text for the uniform LUND creator.
    constexpr auto source = samples::LundSource::Uniform;

    try {
        // Print help only when `--help` is the sole user-supplied argument.
        if ((argc == 2) && (std::string(argv[1]) == "--help")) {
            std::cout << samples::buildHelpText(source);
            return 0;
        }

        // Build and check every setting before generateUniform() may replace the run directory.
        samples::generateUniform(samples::RunConfig::createFromCommandLine(argc, argv, source));

        // All requested output and the completion manifest were written.
        return 0;
    } catch (const std::exception& error) {
        // Print one standard project error and return failure to the calling shell.
        std::cerr << env::ERROR_COLOR << "Error:" << env::RESET_COLOR << ' ' << error.what() << '\n';

        return 1;
    }
}
#pragma endregion
