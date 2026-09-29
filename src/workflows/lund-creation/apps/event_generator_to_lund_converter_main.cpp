//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file event_generator_to_lund_converter_main.cpp
 * @brief Starts the physical LUND converter from the command line.
 *
 * Purpose:
 *   Turn command-line options into checked settings, then copy supported events from an existing
 *   event-generator file into LUND files. This program does not run the event generator itself.
 *
 * Execution flow:
 *   If `--help` is the only option, print the help text and stop successfully. Otherwise, read and check
 *   every option before any output directory can be replaced. Select the input-format reader and run the
 *   physical LUND converter. Return success when it finishes, or print one standard error and return
 *   failure when an exception is reported.
 *
 * CLI options:
 *   --config FILE                       Read `key = value` settings (default: no configuration file).
 *   --input GST_GLOB                    Select GENIE GST ROOT input files (required).
 *   --event-generator genie-gst         Select the physical-input adapter (default: genie-gst).
 *   --event-generator-version VERSION   Record the event-generator version (default: unknown).
 *   --tune NAME                         Record the GENIE tune (default: auto from input_options.txt, then unknown).
 *   --q2-cut NAME                       Record the input selection label (default: auto by beam energy; no cut applied).
 *   --beam-energy GeV                   Set the beam energy written in each LUND header (default: 5.98636 GeV).
 *   --target ID                         Select the target material (default: Ar40).
 *   --gemc-target-variation NAME        Select a compatible GEMC target setup (default: auto).
 *   --A N                               Set the LUND target A value (default: selected target's A).
 *   --Z N                               Set the LUND target Z value (default: selected target's Z).
 *   --output DIRECTORY                  Select the parent output directory (required).
 *   --output-layout nested|metadata     Select the physical run-directory layout (default: nested).
 *   --events N                          Set the maximum number of supported events to write (required).
 *   --events-per-file N                 Split after N written events and require N inclusive input entries before a later file (default: 10000).
 *   --seed N                            Record the particle-motion seed; physical conversion does not use it (default: 67890).
 *   --vertex-seed N                     Set the vertex-position seed (default: 12345; 0 asks ROOT for a new seed).
 *   --prefix NAME                       Set the LUND filename prefix (default: auto from run settings).
 *   --help                              Print this complete option summary when used alone.
 *
 * Output:
 *   The program replaces the fully checked final run directory and writes split LUND files and
 *   `lund-creation-log.json`. The physical LUND converter creates no ROOT monitoring file and does not
 *   start detector simulation.
 */

#include <exception>
#include <iostream>
#include <string>

#include "event-generator-to-lund-converter/PhysicalConverter.h"
#include "support/environment.h"

namespace env = environment;

// Command-line entry point ----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Command-line entry point */
/**
 * @brief Run the physical LUND converter from command-line arguments.
 *
 * Purpose:
 *   Keep command-line help and final error printing in one place. The physical LUND converter handles the
 *   event data and output files.
 *
 * Execution flow:
 *   Show help when requested. Otherwise, read and check the settings, call the physical LUND converter,
 *   return 0 after success, or print the caught error and return 1.
 *
 * @param argc Number of command-line strings, including the executable name.
 * @param argv Command-line strings read during this call. main() does not change or retain them.
 * @return 0 after printing help or completing conversion; 1 when setting preparation or physical LUND
 *         conversion reports a standard C++ exception.
 * @note Exceptions that do not derive from std::exception are not caught and may leave this function.
 */
int main(int argc, char** argv) {
    // Use the settings and help text for existing physical event-generator input.
    constexpr auto source = samples::LundSource::Physical;

    try {
        // Print help only when `--help` is the sole user-supplied argument.
        if ((argc == 2) && (std::string(argv[1]) == "--help")) {
            std::cout << samples::buildHelpText(source);
            return 0;
        }

        // Build and check every setting before conversion may replace the run directory.
        samples::convertPhysical(samples::RunConfig::createFromCommandLine(argc, argv, source));

        // All requested output and the completion manifest were written.
        return 0;
    } catch (const std::exception& error) {
        // Print one standard project error and return failure to the calling shell.
        std::cerr << env::ERROR_COLOR << "Error:" << env::RESET_COLOR << ' ' << error.what() << '\n';

        return 1;
    }
}
#pragma endregion
