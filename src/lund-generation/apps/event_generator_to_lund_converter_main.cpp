//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file event_generator_to_lund_converter_main.cpp
 * @brief Starts physical-event conversion from the command line.
 *
 * Purpose:
 *   Read the selected physical input, run its converter, and return success or failure.
 *
 * Workflow:
 *   Print help when requested -> read and check the options -> convert the events -> return success or
 *   print a caught error.
 *
 * CLI options:
 *   --config FILE                     Read `key = value` settings; CLI values take precedence.
 *   --event-generator genie-gst       Select the GENIE GST input converter (default/currently supported: genie-gst).
 *   --input GST_GLOB                  Required GENIE GST ROOT input file or glob.
 *   --beam-energy GeV                 Set beam energy metadata (default: 5.98636 GeV).
 *   --rgm-target ID                   Select nuclear metadata and automatic geometry (default: Ar40).
 *   --target GEOMETRY                 Override the target-geometry key (default: auto).
 *   --A N / --Z N                     Override LUND target metadata (default: auto).
 *   --output DIRECTORY                Required parent directory for the resolved run directory.
 *   --events N                        Required maximum number of accepted events to write.
 *   --events-per-file N               Split after N accepted events and require that many inclusive input
 *                                     entries before starting a follow-up file (default: 10000).
 *   --seed N / --vertex-seed N        Set configured kinematic/vertex seeds (defaults: 67890/12345).
 *   --prefix NAME                     Override the automatic LUND filename prefix.
 *   --event-generator-version VERSION Record the generator version (default: unknown).
 *   --tune NAME                       Record the generator tune (default: unknown).
 *   --q2-cut NAME                     Record the input selection label (default: auto; no cut is applied here).
 *   --gemc-version VERSION            Record the intended GEMC version (default: unknown).
 *   --gemc-target-variation NAME      Record detector target variation (default: auto).
 *   --help                            Print the complete runtime option summary.
 *
 * Output:
 *   A replaced metadata-named run directory containing split LUND files and a record of the settings.
 */

#include <exception>
#include <iostream>
#include <string>

#include "event-generator-to-lund-converter/PhysicalConverter.h"
#include "support/environment.h"

namespace env = environment;

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Run physical-event conversion from command-line arguments.
 *
 * Purpose:
 *   Handle help and errors here while the selected converter handles event data.
 *
 * Workflow:
 *   Read and check the settings, call the selected converter, and return status 1 when a standard
 *   exception is caught.
 *
 * @param argc Number of executable arguments.
 * @param argv Paths and options supplied by the caller.
 *
 * @return `0` after help or successful conversion; `1` when parsing or conversion throws a
 *         `std::exception`.
 *
 * @throws Nothing for standard parser/converter failures because they are caught here. Non-standard
 *         exceptions are outside this boundary.
 */
int main(int argc, char** argv) {
    constexpr bool uniform = false;
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << samples::help(uniform);
            return 0;
        }
        samples::convertPhysical(samples::RunConfig::parse(argc, argv, uniform));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << env::ERROR_COLOR << "Error: " << env::RESET_COLOR << error.what() << '\n';
        return 1;
    }
}
#pragma endregion
