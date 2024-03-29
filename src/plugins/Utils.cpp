#include <fplus/fplus.hpp>
#include <fstream>
#include <sstream>
#include <Utils.hpp>

bool hasEnum(uint enum_, const TuxClocker::Device::EnumerationVec &enumVec) {
	for (auto &e : enumVec)
		if (e.key == enum_)
			return true;
	return false;
}

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

std::vector<std::string> fileWords(const std::string &path) {
	auto contents = fileContents(path);
	if (!contents.has_value())
		return {};

	return fplus::split_one_of(std::string{"\n "}, false, *contents);
}
