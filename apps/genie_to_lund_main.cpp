//
// Created by Alon Sportes on 26/02/2026.
//

/**
 * @file genie_to_lund_main.cpp
 * @brief GENIE-converter command-line entry point.
 *
 * Purpose:
 *   Translate CLI settings into one conversion call and a process exit status.
 *
 * Workflow:
 *   Help returns immediately; otherwise parse -> convertGenie -> report success or caught failure.
 */

#include <exception>
#include <iostream>
#include <string>

#include "genie/GenieConverter.h"
#include "common/environment.h"

namespace env = environment;

// main ----------------------------------------------------------------------

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
        samples::convertGenie(samples::RunConfig::parse(argc, argv, uniform));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
#pragma endregion
