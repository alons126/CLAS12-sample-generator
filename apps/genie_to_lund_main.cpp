#include <exception>
#include <iostream>
#include <string>

#include "genie/GenieConverter.h"
int main(int argc, char** argv) {
    constexpr bool genie = true;
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << samples::help(genie);
            return 0;
        }
        samples::convertGenie(samples::RunConfig::parse(argc, argv, genie));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
