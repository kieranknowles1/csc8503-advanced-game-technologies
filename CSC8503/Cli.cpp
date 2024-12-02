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
    // Same as consumeArg, but throws an exception if there are no more arguments
    auto consumeRequiredArg = [&](std::string& out) {
		if (!consumeArg(out)) {
			throw std::runtime_error("Expected argument");
		}
	};

    std::string arg;
    while (consumeArg(arg)) {
        if (arg == "-h" || arg == "--help") {
            // Normally we only want to exit by returning from main
            // But I consider printing the help message a special case
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "-i" || arg == "--ip") {
            std::string ipStr;
            char a; char b; char c; char d;
            consumeRequiredArg(ipStr);
            if (sscanf_s(ipStr.c_str(), "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) != 4) {
				throw std::runtime_error("Invalid IP address: " + ipStr);
			}
            ip = (d << 24) | (c << 16) | (b << 8) | a;
        }
        else if (arg == "-s" || arg == "--server") {
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
        "  -i, --ip [address]  Set the IP address to connect to\n"
        "  -s, --server  Start the program as a server\n";
}
