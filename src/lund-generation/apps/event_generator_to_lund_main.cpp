//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file event_generator_to_lund_main.cpp
 * @brief Generator-independent physical conversion command-line entry point.
 *
 * Purpose:
 *   Select the configured physical event-generator adapter and return its process status.
 *
 * Workflow:
 *   Help returns immediately; otherwise parse -> convertGenie -> report success or caught failure.
 *
 * CLI options:
 *   --config FILE                     Read `key = value` settings; CLI values take precedence.
 *   --event-generator genie-gst       Select the GENIE GST adapter (default/currently supported: genie-gst).
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
 *   --event-generator-version VERSION Record generator-version provenance (default: unknown).
 *   --tune NAME                       Record generator-tune provenance (default: unknown).
 *   --q2-cut NAME                     Record the input selection label (default: auto; no cut is applied here).
 *   --gemc-version VERSION            Record intended GEMC-version provenance (default: unknown).
 *   --gemc-target-variation NAME      Record detector target variation (default: auto).
 *   --help                            Print the authoritative runtime option summary.
 *
 * Output:
 *   A replaced metadata-named run directory containing split LUND files and conversion provenance.
 */

#include <exception>
#include <iostream>
#include <string>

#include "core/support/environment.h"
#include "event-generator-to-lund-converter/PhysicalConverter.h"

namespace env = environment;

// main ------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief GENIE-converter command-line entry point.
 *
 * Algorithm:
 *   Translate CLI settings into one conversion call and a process exit status.
 *
 * @param argc Number of executable arguments.
 * @param argv Paths and options supplied by the caller.
 *
 * @return Zero on success; nonzero for a failed run, invalid invocation or test mismatch.
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
