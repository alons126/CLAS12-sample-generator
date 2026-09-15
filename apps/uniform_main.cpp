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

#include "common/environment.h"
#include "uniform/UniformGenerator.h"

namespace env = environment;

// main ----------------------------------------------------------------------

#pragma region /* main */
/**
 * @brief Uniform-generator command-line entry point.
 *
 * Algorithm:
 *   Translate CLI settings into one generation call and a process exit status.
 *
 * @param argc Number of executable arguments.
 * @param argv Paths and options supplied by the caller.
 *
 * @return Zero on success; nonzero for a failed run, invalid invocation or test mismatch.
 */
int main(int argc, char** argv) {
    constexpr bool genie = false;

    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << samples::help(genie);
            return 0;
        }

        samples::generateUniform(samples::RunConfig::parse(argc, argv, genie));

        return 0;
    } catch (const std::exception& error) {
        std::cerr << env::ERROR_COLOR << "Error: " << env::RESET_COLOR << error.what() << '\n';

        return 1;
    }
}
#pragma endregion
