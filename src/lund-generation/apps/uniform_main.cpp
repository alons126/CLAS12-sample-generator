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
