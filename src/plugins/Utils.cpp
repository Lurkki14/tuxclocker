#include <fstream>
#include <sstream>
#include <Utils.hpp>

bool hasReadableValue(TuxClocker::Device::ReadResult res) {
	return std::holds_alternative<TuxClocker::Device::ReadableValue>(res);
}

std::optional<std::string> fileContents(const std::string &path) {
	std::ifstream file{path};
	if (!file.good())
		return std::nullopt;

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}
