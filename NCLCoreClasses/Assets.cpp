#include "Assets.h"

using namespace NCL;

void Assets::ReadTextFile(const std::string &filepath, std::string& result) {
	std::ifstream file(filepath, std::ios::in);
	if (!file) {
		throw std::runtime_error("File not found: " + filepath);
	}
	std::ostringstream stream;

	stream << file.rdbuf();

	result = stream.str();
}

bool	Assets::ReadBinaryFile(const std::string& filename, char** into, size_t& size) {
	std::ifstream file(filename, std::ios::binary);

	if (!file) {
		return false;
	}

	file.seekg(0, std::ios_base::end);

	std::streamoff filesize = file.tellg();

	file.seekg(0, std::ios_base::beg);

	char* data = new char[(unsigned int)filesize];

	file.read(data, filesize);

	file.close();

	*into = data;
	size = filesize;

	return data != NULL ? true : false;
}
