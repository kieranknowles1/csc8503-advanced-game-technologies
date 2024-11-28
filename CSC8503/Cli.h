#pragma once

// Helper to parse command line arguments
// Throws std::runtime_error if an unknown argument is encountered
// Exits the program if the help flag is found
class Cli {
public:
	Cli(int argc, char** argv);

	bool isServer() const {
		// TODO: Don't invert this
		// TODO: Script to run both server and client
		return !server;
	}

private:
	bool server = false;

    void printUsage(char* programName) const;
};
