#pragma once

#include "NetworkBase.h"
#include "Vector.h"

// Helper to parse command line arguments
// Throws std::runtime_error if an unknown argument is encountered
// Exits the program if the help flag is found
class Cli {
public:
	enum class ClientType {
		Auto, // Prompt the user
		Client,
		Server,
	};

	Cli(int argc, char** argv);

	ClientType getClientType() const {
		return clientType;
	}

	bool shouldCaptureMouse() const {
		return captureMouse;
	}

	bool isFullscreen() const {
		return fullscreen;
	}

	uint32_t getIp() const {
		return ip;
	}

	NCL::Maths::Vector2i getWindowPos() const {
		return windowPos;
	}

	std::string_view getName() const {
		return name;
	}

	float getMaxGameLength() const {
		return maxGameLength;
	}
private:
	ClientType clientType = ClientType::Auto;
	bool captureMouse = true;
	bool fullscreen = false;
	uint32_t ip = NetworkBase::ipv4(127, 0, 0, 1); // Default to localhost

	float maxGameLength = 300.0f;

	NCL::Maths::Vector2i windowPos = NCL::Maths::Vector2i(0, 0);

	std::string name = "User McUserface";

    void printUsage(char* programName) const;
};
