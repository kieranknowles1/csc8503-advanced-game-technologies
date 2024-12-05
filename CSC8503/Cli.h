#pragma once

#include "NetworkBase.h"
#include "Vector.h"

// Helper to parse command line arguments
// Throws std::runtime_error if an unknown argument is encountered
// Exits the program if the help flag is found
class Cli {
public:
	Cli(int argc, char** argv);

	bool isClient() const {
		return client;
	}

	uint32_t getIp() const {
		return ip;
	}

	NCL::Maths::Vector2i getWindowPos() const {
		return windowPos;
	}
private:
	bool client = false;
	uint32_t ip = NetworkBase::ipv4(127, 0, 0, 1); // Default to localhost

	NCL::Maths::Vector2i windowPos = NCL::Maths::Vector2i(0, 0);

    void printUsage(char* programName) const;
};
