#include "Cli.h"

#include <string>
#include <stdexcept>
#include <iostream>

#ifndef _WIN32
#define sscanf_s sscanf
#endif

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
            ip = NetworkBase::ipv4(a, b, c, d);
        }
        else if (arg == "-c" || arg == "--client") {
            clientType = ClientType::Client;
        } else if (arg == "-s" || arg == "--server") {
            clientType = ClientType::Server;
        } else if (arg == "-C" || arg == "--no-capture") {
            captureMouse = false;
        } else if (arg == "-f" || arg == "--fullscreen") {
            fullscreen = true;
        } else if (arg == "-w" || arg == "--window") {
            std::string xStr; std::string yStr;
            consumeRequiredArg(xStr);
            consumeRequiredArg(yStr);
            int x; int y;
            if (sscanf_s(xStr.c_str(), "%d", &x) != 1 || sscanf_s(yStr.c_str(), "%d", &y) != 1) {
                throw std::runtime_error("Invalid window position: " + xStr + " " + yStr);
            }
            windowPos = NCL::Maths::Vector2i(x, y);
        }
        else if (arg == "-n" || arg == "--name") {
            consumeRequiredArg(name);
        } else if (arg == "-t" || arg == "--time-limit") {
            std::string timeStr;
			consumeRequiredArg(timeStr);
			if (sscanf_s(timeStr.c_str(), "%f", &maxGameLength) != 1) {
				throw std::runtime_error("Invalid time limit: " + timeStr);
			}
        } else {
            throw std::runtime_error("Unknown argument: " + arg + " (try -h for help)");
        }
    }
}

// Naming convention for arguments: single char flag with single hyphen, multi char flag with double hyphen
// Negations are done with "no-" prefix and a caputalized flag name
void Cli::printUsage(char* programName) const {
    std::cout <<
        "Usage: " << programName << " [options]\n"
        "  -h, --help                    Display this help message\n"
        "  -i, --ip [address=127.0.0.1]  Set the IP address to connect to\n"
        "  -c, --client                  Run the program as a client\n"
        "  -s, --server                  Run the program as a server\n"
        "  -C, --no-capture              Don't capture the mouse\n"
        "  -f, --fullscreen              Run the program in fullscreen\n"
        "  -w, --window [x=0] [y=0]      Set the window position\n"
        "  -t, --time-limit              Set the maximum game length\n"
        "  -n, --name [name=User]        Set the user name\n";
}
