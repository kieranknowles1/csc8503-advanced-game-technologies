#pragma once

#include "NetworkBase.h"

// Helper to parse command line arguments
// Throws std::runtime_error if an unknown argument is encountered
// Exits the program if the help flag is found
class Cli {
public:
	Cli(int argc, char** argv);

	bool isServer() const {
		// TODO: Don't invert this
		// TODO: Script to run both server and client
		return server;
	}

	uint32_t getIp() const {
		return ip;
	}
private:
	bool server = false;
	uint32_t ip = NetworkBase::ipv4(127, 0, 0, 1); // Default to localhost

    void printUsage(char* programName) const;
};
