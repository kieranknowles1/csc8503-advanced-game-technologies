#include "Cli.h"

#include <string>
#include <stdexcept>
#include <iostream>

Cli::Cli(int argc, char** argv) {
    int i = 1; // Skip the first argument, which is the program name
    // Read and consume the next argument
    // Returns false if there are no more arguments
    auto consumeArg = [&](std::string& out) {
        if (i >= argc) return false;
        out = argv[i];
        i++;
        return true;
    };

    std::string arg;
    while (consumeArg(arg)) {
        if (arg == "-h" || arg == "--help") {
            // Normally we only want to exit by returning from main
            // But I consider printing the help message a special case
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "-s" || arg == "--server") {
            server = true;
        } else {
            throw std::runtime_error("Unknown argument: " + arg + " (try -h for help)");
        }
    }
}

void Cli::printUsage(char* programName) const {
    std::cout <<
        "Usage: " << programName << " [options]\n"
        "  -h, --help    Display this help message\n"
        "  -s, --server  Start the program as a server\n";
}
