#include <Crypto.hpp>
#include <filesystem>
#include <fplus/fplus.hpp>
#include <fstream>
#include <libintl.h>
#include <Plugin.hpp>
#include <regex>
#include <TreeConstructor.hpp>
#include <Utils.hpp>

#define _(String) gettext(String)

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

// TODO: this might be useful for other hwmon stuff too
std::optional<std::string> coretempHwmonPath() {
	auto hwmonDirs = std::filesystem::directory_iterator("/sys/class/hwmon");
	for (auto &dir : hwmonDirs) {
		// See if 'name' file contains 'coretemp'
		auto namePath = dir.path().string() + "/name";
		std::ifstream file{namePath};
		if (file.good()) {
			std::stringstream buf;
			buf << file.rdbuf();

			if (buf.str().find("coretemp") != std::string::npos)
				return dir.path().string();
		}
	}
	return std::nullopt;
}

std::optional<DynamicReadable> frequencyReadable(uint coreIndex) {
	char path[64];
	snprintf(path, 64, "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_cur_freq", coreIndex);

	std::ifstream file{path};
	if (!file.good())
		return std::nullopt;

	auto func = [=]() -> ReadResult {
		std::ifstream file{path};
		if (!file.good())
			return ReadError::UnknownError;
		std::stringstream buffer;
		buffer << file.rdbuf();

		auto value = static_cast<uint>(std::stoi(buffer.str()));
		// kHz -> MHz
		return value / 1000;
	};

	return DynamicReadable{func, _("MHz")};
}

std::optional<DynamicReadable> coretempReadable(const char *hwmonPath, uint index) {
	char path[64];
	snprintf(path, 64, "%s/temp%u_input", hwmonPath, index);

	auto func = [=]() -> ReadResult {
		std::ifstream file{path};
		if (!file.good())
			return ReadError::UnknownError;
		std::stringstream buffer;
		buffer << file.rdbuf();

		auto value = static_cast<uint>(std::stoi(buffer.str()));
		// millicelcius -> celcius
		return value / 1000;
	};

	if (hasReadableValue(func()))
		return DynamicReadable{func, _("°C")};
	return std::nullopt;
}

std::vector<TreeNode<DeviceNode>> getCoretempTemperatures(CPUData data) {
	// Temperature nodes for Intel CPUs

	// There seem to be coreCount + 1 files, where the first is
	// called 'Package id 0' meaning overall temp, rest are for individual cores
	auto hwmonPathO = coretempHwmonPath();
	if (!hwmonPathO.has_value())
		return {};

	auto hwmonPath = hwmonPathO.value().c_str();
	std::vector<TreeNode<DeviceNode>> retval;
	// Get max (slowdown) temp, assumed to be the same for all cores
	char maxTempPath[64];
	snprintf(maxTempPath, 64, "%s/temp%u_crit", hwmonPath, data.firstCoreIndex + 1);

	std::ifstream file{maxTempPath};
	if (file.good()) {
		std::stringstream buf;
		buf << file.rdbuf();

		StaticReadable sr{static_cast<uint>(std::stoi(buf.str())) / 1000, "°C"};

		DeviceNode node{
		    .name = _("Slowdown Temperature"),
		    .interface = sr,
		    .hash = md5(data.identifier + "Slowdown Temperature"),
		};
		retval.push_back(node);
	}

	// Indices start at 1
	auto overallTemp = coretempReadable(hwmonPath, data.firstCoreIndex + 1);
	if (overallTemp.has_value()) {
		DeviceNode node{
		    .name = _("Overall Temperature"),
		    .interface = overallTemp.value(),
		    .hash = md5(data.identifier + "Overall Temperature"),
		};
		retval.push_back(node);
	}

	// Nodes for individual cores
	auto firstId = data.firstCoreIndex + 2;
	for (uint i = firstId; i < firstId + data.coreCount; i++) {
		auto dr = coretempReadable(hwmonPath, i);
		if (dr.has_value()) {
			auto realId = i - 2;
			char idStr[64];
			snprintf(idStr, 64, "%sCore%uTemperature", data.identifier.c_str(), realId);

			auto coreStr = _("Core");
			char name[32];
			snprintf(name, 32, "%s %u", coreStr, realId);
			DeviceNode node{
			    .name = name,
			    .interface = overallTemp.value(),
			    .hash = md5(idStr),
			};
			retval.push_back(node);
		}
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getFreqs(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;
	// Try to get DynamicReadable for all cores
	for (uint i = data.firstCoreIndex; i < data.firstCoreIndex + data.coreCount; i++) {
		auto dr = frequencyReadable(i);
		if (dr.has_value()) {
			char idStr[64];
			snprintf(idStr, 64, "%sCore%uFrequency", data.identifier.c_str(), i);

			auto coreStr = _("Core");
			char name[32];
			snprintf(name, 32, "%s %u", coreStr, i);
			DeviceNode node{
			    .name = name,
			    .interface = dr.value(),
			    .hash = md5(idStr),
			};
			retval.push_back(node);
		}
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getFreqsRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Frequencies"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Frequencies"),
	}};
}

std::vector<TreeNode<DeviceNode>> getTemperaturesRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Temperatures"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Temperatures"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCPUName(CPUData data) {
	return {DeviceNode{
	    .name = data.name,
	    .interface = std::nullopt,
	    .hash = md5(data.identifier),
	}};
}

// Temperatures: /sys/class/hwmon/hwmonX where =||=/name = coretemp
// Frequencies: /sys/devices/system/cpu/cpuX/cpufreq/ where X is the
// Utilizations: /proc/stat ?
// 'processor' field in /proc/cpuinfo

// clang-format off
auto cpuTree = TreeConstructor<CPUData, DeviceNode>{
	getCPUName, {
		{getFreqsRoot, {
			{getFreqs, {}}
		}},
		{getTemperaturesRoot, {
			{getCoretempTemperatures, {}},
		}}
	}
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
