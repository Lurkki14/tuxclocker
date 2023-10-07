#include <Crypto.hpp>
#include <fplus/fplus.hpp>
#include <fstream>
#include <Plugin.hpp>
#include <regex>
#include <TreeConstructor.hpp>

using namespace fplus;

using namespace TuxClocker;
using namespace TuxClocker::Crypto;
using namespace TuxClocker::Device;
using namespace TuxClocker::Plugin;

// Data that we parse from various places
// /proc/cpuinfo
struct CPUInfoData {
	uint processor; // meaning core
	uint family;
	uint model;
	std::string name;
	uint physicalId;
	uint cores;
};

// What we use to construct the device tree
struct CPUData {
	// index/family/model/, eg. 0/6/158/
	std::string identifier;
	// This might be useful assuming systems with multiple CPUs
	// cram all cores into the same folder
	uint firstCoreIndex;
	uint coreCount;
	std::string name;
};

std::vector<CPUData> fromCPUInfoData(std::vector<CPUInfoData> dataVec) {
	auto samePhysId = [](CPUInfoData a, CPUInfoData b) { return a.physicalId == b.physicalId; };
	auto cpus = group_by(samePhysId, dataVec);

	auto smallerCoreId = [](CPUInfoData a, CPUInfoData b) { return a.processor < b.processor; };
	std::vector<CPUData> retval;
	for (auto &cpu : cpus) {
		auto firstCore = minimum_by(smallerCoreId, cpu).processor;
		auto first = cpu.front();
		// Create identifier
		char identBuf[20];
		snprintf(identBuf, 20, "%u/%u/%u/", first.physicalId, first.family, first.model);
		CPUData data{
		    .identifier = identBuf,
		    .firstCoreIndex = firstCore,
		    .coreCount = first.cores,
		    .name = first.name,
		};
		retval.push_back(data);
	}
	return retval;
}

std::optional<std::string> parseCPUInfoLine(std::string title, std::string line) {
	// Check if line starts with title
	if (line.rfind(title, 0) != 0)
		return std::nullopt;

	// Found title, the string we need is everything after the next ': '
	std::regex valueEx{"\\:\\s(.*)"};
	std::smatch matches;
	if (std::regex_search(line, matches, valueEx) && !matches.empty()) {
		// First capturing group is the string we want
		std::string s = matches.str(1);
		return s;
	}
	return std::nullopt;
}

std::vector<std::string> splitAt(std::string delimiter, std::string input) {
	std::vector<std::string> retval;
	size_t pos;
	std::string token;
	while ((pos = input.find(delimiter)) != std::string::npos) {
		token = input.substr(0, pos);
		retval.push_back(token);
		input.erase(0, pos + delimiter.length());
	}
	return retval;
}

std::optional<CPUInfoData> parseCPUInfoSection(std::string section) {
	auto lines = splitAt("\n", section);

	std::vector<std::string> titlesOrdered{
	    "processor", "cpu family", "model", "model name", "physical id", "cpu cores"};
	std::vector<std::string> parsed;
	auto titlesSize = titlesOrdered.size();
	// Assumes /proc/cpuinfo is ordered the same way everywhere
	for (size_t i = 0, titleId = 0; i < lines.size() && titleId < titlesSize; i++) {
		auto value = parseCPUInfoLine(titlesOrdered[titleId], lines[i]);
		if (value.has_value()) {
			parsed.push_back(value.value());
			titleId++;
		}
	}

	if (parsed.size() != titlesOrdered.size())
		return std::nullopt;

	return CPUInfoData{
	    .processor = static_cast<uint>(std::stoi(parsed[0])),
	    .family = static_cast<uint>(std::stoi(parsed[1])),
	    .model = static_cast<uint>(std::stoi(parsed[2])),
	    .name = parsed[3],
	    .physicalId = static_cast<uint>(std::stoi(parsed[4])),
	    .cores = static_cast<uint>(std::stoi(parsed[5])),
	};
}

std::vector<CPUInfoData> parseCPUInfo() {
	std::ifstream file{"/proc/cpuinfo"};
	std::stringstream buffer;
	buffer << file.rdbuf();

	auto contents = buffer.str();
	// Split into paragraphs/sections
	auto sections = splitAt("\n\n", contents);
	std::vector<CPUInfoData> retval;

	for (auto &section : sections) {
		auto data = parseCPUInfoSection(section);
		if (data.has_value())
			retval.push_back(data.value());
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getCPUName(CPUData data) {
	return {DeviceNode{
	    .name = data.name,
	    .hash = md5(data.identifier),
	}};
}

// Temperatures: /sys/class/hwmon/hwmonX where =||=/name = coretemp
// Frequencies: /sys/devices/system/cpu/cpuX/cpufreq/ where X is the
// 'processor' field in /proc/cpuinfo

// clang-format off
auto cpuTree = TreeConstructor<CPUData, DeviceNode>{
	getCPUName, {}
};
// clang-format on

class CPUPlugin : public DevicePlugin {
public:
	CPUPlugin() {}
	~CPUPlugin() {}
	TreeNode<DeviceNode> deviceRootNode() {
		auto cpuInfoList = parseCPUInfo();
		auto cpuDataList = fromCPUInfoData(cpuInfoList);

		TreeNode<DeviceNode> root{};

		for (auto &cpuData : cpuDataList)
			constructTree<CPUData, DeviceNode>(cpuTree, root, cpuData);

		return root;
	}
	std::optional<InitializationError> initializationError() { return std::nullopt; }
};

TUXCLOCKER_PLUGIN_EXPORT(CPUPlugin)
