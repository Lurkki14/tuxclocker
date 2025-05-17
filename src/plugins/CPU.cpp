#include <Crypto.hpp>
#include <fcntl.h>
#include <filesystem>
#include <fplus/fplus.hpp>
#include <fstream>
#include <libintl.h>
#include <Plugin.hpp>
#include <regex>
#include <sys/time.h>
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
	std::string vendorId;
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
	uint cpuIndex;
	std::string vendorId;
};

// CPU utilization sample
struct CPUTimeStat {
	uint64_t totalTime;
	uint64_t idleTime;
};

// Used to calculate power usage
struct EnergyState {
	uint64_t counter;
	uint64_t usecs;
};

uint utilizationPercentage(CPUTimeStat stat) {
	auto idle = static_cast<double>(stat.idleTime) / static_cast<double>(stat.totalTime);
	auto active = 1 - idle;
	return std::round(active * 100);
}

std::optional<CPUTimeStat> fromStatLine(const std::string &line) {
	auto words = fplus::split(' ', true, line);
	// Remove cpuX from the start
	words.erase(words.begin());

	if (words.size() < 4)
		return std::nullopt;

	uint64_t total = 0;
	for (auto &word : words) {
		total += std::stoull(word);
	}
	return CPUTimeStat{
	    .totalTime = total,
	    .idleTime = std::stoull(words[3]),
	};
}

std::optional<uint64_t> readMsr(uint64_t address, uint64_t mask, uint coreIndex) {
	char path[32];
	snprintf(path, 32, "/dev/cpu/%u/msr", coreIndex);

	int msr_fd = open(path, O_RDONLY);
	if (msr_fd < 0)
		return std::nullopt;

	uint64_t reg_value;
	auto status = pread(msr_fd, &reg_value, sizeof(reg_value), address);
	close(msr_fd);
	if (status < 1)
		return std::nullopt;

	return reg_value & mask;
}

std::ifstream &jumptoLine(std::ifstream &file, unsigned int num) {
	file.seekg(std::ios::beg);
	for (int i = 0; i < num - 1; ++i) {
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return file;
}

std::vector<CPUData> fromCPUInfoData(std::vector<CPUInfoData> dataVec) {
	auto samePhysId = [](CPUInfoData a, CPUInfoData b) { return a.physicalId == b.physicalId; };
	auto cpus = group_by(samePhysId, dataVec);

	auto smallerCoreId = [](CPUInfoData a, CPUInfoData b) { return a.processor < b.processor; };
	std::vector<CPUData> retval;
	for (auto &cpu : cpus) {
		// These are actually threads, and not the 'core count' field,
		// since one core may have more than one thread, and threads are used in sysfs
		auto firstCore = minimum_by(smallerCoreId, cpu).processor;
		auto lastCore = fplus::maximum_by(smallerCoreId, cpu).processor;
		auto first = cpu.front();
		// Create identifier
		char identBuf[20];
		snprintf(identBuf, 20, "%u/%u/%u/", first.physicalId, first.family, first.model);
		CPUData data{
		    .identifier = identBuf,
		    .firstCoreIndex = firstCore,
		    .coreCount = (lastCore - firstCore) + 1,
		    .name = first.name,
		    .cpuIndex = first.physicalId,
		    .vendorId = first.vendorId,
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

	std::vector<std::string> titlesOrdered{"processor", "vendor_id", "cpu family", "model",
	    "model name", "physical id", "cpu cores"};
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
	    .vendorId = parsed[1],
	    .family = static_cast<uint>(std::stoi(parsed[2])),
	    .model = static_cast<uint>(std::stoi(parsed[3])),
	    .name = parsed[4],
	    .physicalId = static_cast<uint>(std::stoi(parsed[5])),
	    .cores = static_cast<uint>(std::stoi(parsed[6])),
	};
}

std::vector<CPUInfoData> parseCPUInfo() {
	auto contents = fileContents("/proc/cpuinfo");
	if (!contents.has_value())
		return {};

	// Split into paragraphs/sections
	auto sections = splitAt("\n\n", *contents);
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
		auto contents = fileContents(namePath);
		if (contents.has_value() && contents->find("coretemp") != std::string::npos)
			return dir.path().string();
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
		auto contents = fileContents(path);
		if (!contents.has_value())
			return ReadError::UnknownError;

		auto value = static_cast<uint>(std::stoi(*contents));
		// kHz -> MHz
		return value / 1000;
	};

	return DynamicReadable{func, _("MHz")};
}

std::optional<DynamicReadable> coretempReadable(const char *hwmonPath, uint index) {
	char path[64];
	snprintf(path, 64, "%s/temp%u_input", hwmonPath, index);

	auto func = [=]() -> ReadResult {
		auto contents = fileContents(path);
		if (!contents.has_value())
			return ReadError::UnknownError;

		auto value = static_cast<uint>(std::stoi(*contents));
		// millicelcius -> celcius
		return value / 1000;
	};

	if (hasReadableValue(func()))
		return DynamicReadable{func, _("°C")};
	return std::nullopt;
}

std::vector<CPUTimeStat> readCPUStatsFromRange(uint minId, uint maxId) {
	std::ifstream stat{"/proc/stat"};
	if (!stat.good())
		return {};

	std::vector<CPUTimeStat> retval;
	// cpu0 is on the second line
	jumptoLine(stat, minId + 2);
	// TODO: How did I end up with this :D at least debugger shows it works as intended
	for (uint i = minId + 1; i < maxId + 2; i++) {
		std::string line;
		// Seeks to next line
		std::getline(stat, line);

		auto timeStat = fromStatLine(line);
		if (timeStat.has_value())
			retval.push_back(timeStat.value());
		else
			return {};
	}
	return retval;
}

CPUTimeStat timeStatDelta(CPUTimeStat prev, CPUTimeStat cur) {
	return CPUTimeStat{
	    .totalTime = cur.totalTime - prev.totalTime,
	    .idleTime = cur.idleTime - prev.idleTime,
	};
}

// Returns a list of readings from minId to maxId
// This is done in groups like this so we can avoid traversing the file again for every core
std::vector<uint> utilizationsFromRange(uint minId, uint maxId) {
	// Saves the sample so we can compare to it on the next call
	static std::unordered_map<uint, CPUTimeStat> timeStatMap;

	auto newTimeStats = readCPUStatsFromRange(minId, maxId);
	if (newTimeStats.empty())
		return {};

	std::vector<uint> retval;
	// Try to get previous reading from map
	auto first = timeStatMap.find(minId);
	if (first != timeStatMap.end()) {
		// Found previous reading
		for (uint i = 0; i < newTimeStats.size(); i++) {
			try {
				auto coreId = i + minId;
				auto curStat = newTimeStats[i];
				auto prevStat = timeStatMap.at(coreId);

				auto deltaStat = timeStatDelta(prevStat, curStat);
				retval.push_back(utilizationPercentage(deltaStat));
				// Save new value into hashmap
				timeStatMap[coreId] = curStat;
			} catch (std::out_of_range) {
				return {};
			}
		}
	} else {
		// Use overall utilization
		for (uint i = 0; i < newTimeStats.size(); i++) {
			auto coreId = i + minId;
			// Save current stats so we can calculate delta next call
			timeStatMap[coreId] = newTimeStats[i];
			retval.push_back(utilizationPercentage(newTimeStats[i]));
		}
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getIntelEPBNodes(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;
	Range<int> range{0, 15};

	for (uint i = data.firstCoreIndex; i < data.firstCoreIndex + data.coreCount; i++) {
		char path[96];
		snprintf(path, 96, "/sys/devices/system/cpu/cpu%u/power/energy_perf_bias", i);
		std::ifstream file{path};
		if (!file.good())
			continue;

		auto getFunc = [=]() -> std::optional<AssignmentArgument> {
			auto contents = fileContents(path);
			if (!contents.has_value())
				return std::nullopt;
			return std::stoi(*contents);
		};

		auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
			std::ofstream file{path};
			if (!file.good())
				return AssignmentError::UnknownError;

			if (!std::holds_alternative<int>(a))
				return AssignmentError::InvalidType;

			auto arg = std::get<int>(a);
			if (arg < 0 || arg > 15)
				return AssignmentError::OutOfRange;

			file << arg;
			return std::nullopt;
		};

		Assignable a{setFunc, range, getFunc, std::nullopt};

		char idStr[64];
		snprintf(idStr, 64, "%sCore%uEPB", data.identifier.c_str(), i);
		char nameStr[32];
		snprintf(nameStr, 32, "%s %u", _("Core"), i);

		DeviceNode node{
		    .name = nameStr,
		    .interface = a,
		    .hash = md5(idStr),
		};
		retval.push_back(node);
	}
	return retval;
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

	auto contents = fileContents(maxTempPath);
	if (contents.has_value()) {
		StaticReadable sr{static_cast<uint>(std::stoi(*contents)) / 1000, "°C"};

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

ReadResult utilizationBuffered(CPUData data, uint coreId) {
	// NOTE: relies on the looping order in getUtilizations going from low to high
	// Use cached results until we fetch for the first core id again
	static std::unordered_map<uint, std::vector<uint>> cachedUtils;

	auto vectorIndex = coreId - data.firstCoreIndex;
	auto lastId = data.firstCoreIndex + data.coreCount - 1;
	if (cachedUtils.find(data.cpuIndex) != cachedUtils.end()) {
		// Cache has value
		auto utils = cachedUtils[data.cpuIndex];
		// Position relative to CPU
		if (coreId == lastId) {
			// Remove from map so we retrieve new value next time
			cachedUtils.erase(data.cpuIndex);
		}
		return utils[vectorIndex];
	}

	auto utils = utilizationsFromRange(data.firstCoreIndex, lastId);
	if (utils.empty())
		return ReadError::UnknownError;

	cachedUtils.insert({data.cpuIndex, utils});
	return utils[vectorIndex];
}

std::vector<TreeNode<DeviceNode>> getUtilizations(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;
	for (uint i = data.firstCoreIndex; i < data.firstCoreIndex + data.coreCount; i++) {
		auto func = [=]() -> ReadResult { return utilizationBuffered(data, i); };

		if (hasReadableValue(func())) {
			char idStr[64];
			char name[32];
			snprintf(idStr, 64, "%sCore%uUtilization", data.identifier.c_str(), i);
			snprintf(name, 32, "%s %u", _("Core"), i);

			DynamicReadable dr{func, _("%")};

			DeviceNode node{
			    .name = name,
			    .interface = dr,
			    .hash = md5(idStr),
			};
			retval.push_back(node);
		}
	};
	return retval;
}

double energyCounterFactor(CPUData data) {
	static std::unordered_map<uint, double> factors;

	if (factors.find(data.cpuIndex) == factors.end()) {
		// No value yet
		// Bits 8:12
		auto unit = readMsr(0x606, 0x1f00, data.firstCoreIndex);
		if (!unit.has_value())
			// Assume 14 ESU; 61 uJ increment
			factors[data.cpuIndex] = (1 / pow(2, 14));
		else
			factors[data.cpuIndex] = (1 / pow(2, (*unit >> 8)));
	}
	return factors[data.cpuIndex];
}

double toWatts(EnergyState current, EnergyState previous, CPUData data) {
	auto counterDelta = current.counter - previous.counter;
	// us -> s
	double delta_s = (current.usecs - previous.usecs) / 1000000.0;
	double delta_j = (counterDelta * energyCounterFactor(data));
	return delta_j / delta_s;
}

std::optional<EnergyState> getEnergyState(int address, int mask, uint coreIndex) {
	auto curCounter = readMsr(address, mask, coreIndex);
	if (!curCounter.has_value() || *curCounter == 0)
		return std::nullopt;

	timeval time;
	if (gettimeofday(&time, NULL) != 0)
		return std::nullopt;

	uint64_t cur_usecs = (time.tv_sec * 1000000) + time.tv_usec;
	return EnergyState{.counter = *curCounter, .usecs = cur_usecs};
}

// TODO: quite a bit of copypasta, mostly due to every function keeping its own state
std::vector<TreeNode<DeviceNode>> getTotalPowerUsage(CPUData data) {
	std::optional<int> msrAddress = std::nullopt;
	if (data.vendorId == "GenuineIntel")
		// MSR_PKG_ENERGY_STATUS
		msrAddress = 0x611;
	if (data.vendorId == "AuthenticAMD")
		// TODO: we can get per-core power usage for AMD
		msrAddress = 0xc001029b;

	if (!msrAddress.has_value())
		return {};

	auto func = [=]() -> ReadResult {
		// Previous EnergyState for CPU index
		static std::unordered_map<uint, EnergyState> prevStates;

		// 32 bits
		auto curState = getEnergyState(*msrAddress, 0xffffffff, data.firstCoreIndex);
		if (!curState.has_value())
			return ReadError::UnknownError;

		if (prevStates.find(data.cpuIndex) == prevStates.end()) {
			// No previous value
			prevStates[data.cpuIndex] = *curState;
			// This result shouldn't be seen since this is called at initialization
			return 0.0f;
		}
		auto prevState = prevStates[data.cpuIndex];

		prevStates[data.cpuIndex] = *curState;
		return toWatts(*curState, prevState, data);
	};

	if (!hasReadableValue(func()))
		return {};

	DynamicReadable dr{func, _("W")};

	return {DeviceNode{
	    .name = _("Power Usage"),
	    .interface = dr,
	    .hash = md5(data.identifier + "Power Usage"),
	}};
}

std::vector<TreeNode<DeviceNode>> getDramPowerUsage(CPUData data) {
	if (data.vendorId != "GenuineIntel")
		return {};

	auto func = [=]() -> ReadResult {
		// Previous EnergyState for CPU index
		static std::unordered_map<uint, EnergyState> prevStates;

		// MSR_DRAM_ENERGY_STATUS, 32 bits
		auto curState = getEnergyState(0x619, 0xffffffff, data.firstCoreIndex);
		if (!curState.has_value())
			return ReadError::UnknownError;

		if (prevStates.find(data.cpuIndex) == prevStates.end()) {
			// No previous value
			prevStates[data.cpuIndex] = *curState;
			// This result shouldn't be seen since this is called at initialization
			return 0.0f;
		}
		auto prevState = prevStates[data.cpuIndex];

		prevStates[data.cpuIndex] = *curState;
		return toWatts(*curState, prevState, data);
	};

	if (!hasReadableValue(func()))
		return {};

	DynamicReadable dr{func, _("W")};

	return {DeviceNode{
	    .name = _("Memory Power Usage"),
	    .interface = dr,
	    .hash = md5(data.identifier + "DRAM Power Usage"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCorePowerUsage(CPUData data) {
	if (data.vendorId != "GenuineIntel")
		return {};

	auto func = [=]() -> ReadResult {
		// Previous EnergyState for CPU index
		static std::unordered_map<uint, EnergyState> prevStates;

		// MSR_PP0_ENERGY_STATUS, 32 bits
		auto curState = getEnergyState(0x639, 0xffffffff, data.firstCoreIndex);
		if (!curState.has_value())
			return ReadError::UnknownError;

		if (prevStates.find(data.cpuIndex) == prevStates.end()) {
			// No previous value
			prevStates[data.cpuIndex] = *curState;
			// This result shouldn't be seen since this is called at initialization
			return 0.0f;
		}
		auto prevState = prevStates[data.cpuIndex];

		prevStates[data.cpuIndex] = *curState;
		return toWatts(*curState, prevState, data);
	};

	if (!hasReadableValue(func()))
		return {};

	DynamicReadable dr{func, _("W")};

	return {DeviceNode{
	    .name = _("Core Power Usage"),
	    .interface = dr,
	    .hash = md5(data.identifier + "Core Power Usage"),
	}};
}

std::vector<TreeNode<DeviceNode>> getGovernors(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;

	// Convert to our own names so we can localize them
	auto fromSysFsName = [](const std::string &sysFsName) -> std::string {
		if (sysFsName.find("powersave") != std::string::npos)
			return _("Power Saving");
		if (sysFsName.find("performance") != std::string::npos)
			return _("Performance");
		if (sysFsName.find("schedutil") != std::string::npos)
			return _("Scheduler Controlled");
		// Unknown name
		return sysFsName;
	};

	for (uint i = data.firstCoreIndex; i < data.firstCoreIndex + data.coreCount; i++) {
		char path[96];
		snprintf(path, 96,
		    "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_available_governors", i);

		auto contents = fileContents(path);
		if (!contents.has_value())
			continue;

		EnumerationVec enumVec;
		std::vector<std::string> sysFsNames;
		int enumId = 0;
		for (auto &word : split_words(false, *contents)) {
			auto e = Enumeration{fromSysFsName(word), static_cast<unsigned>(enumId)};
			enumId++;
			enumVec.push_back(e);
			sysFsNames.push_back(word);
		}
		char curPath[96];
		snprintf(curPath, 96, "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_governor", i);

		auto getFunc = [=]() -> std::optional<AssignmentArgument> {
			auto string = fileContents(curPath);
			if (!string.has_value())
				return std::nullopt;

			for (int i = 0; i < enumVec.size(); i++) {
				if (string->find(sysFsNames[i]) != std::string::npos)
					return enumVec[i].key;
			}
			return std::nullopt;
		};

		auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
			std::ofstream file{curPath};
			if (!file.good())
				return AssignmentError::UnknownError;

			if (!std::holds_alternative<uint>(a))
				return AssignmentError::InvalidType;

			auto arg = std::get<uint>(a);
			if (!hasEnum(arg, enumVec))
				return AssignmentError::OutOfRange;

			if (file << sysFsNames[arg])
				return std::nullopt;
			return AssignmentError::UnknownError;
		};

		Assignable a{setFunc, enumVec, getFunc, std::nullopt};

		char idStr[64];
		snprintf(idStr, 64, "%sCore%uGovernor", data.identifier.c_str(), i);
		char name[32];
		snprintf(name, 32, "%s %u", _("Core"), i);

		if (getFunc().has_value()) {
			DeviceNode node{
			    .name = name,
			    .interface = a,
			    .hash = md5(idStr),
			};
			retval.push_back(node);
		}
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getEPPNodes(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;

	auto fromSysFsName = [](const std::string &sysFsName) -> std::string {
		if (sysFsName == "performance")
			return _("Performance");
		if (sysFsName == "balance_performance")
			return _("Balanced Performance");
		if (sysFsName == "default")
			return _("Default");
		if (sysFsName == "balance_power")
			return _("Balanced Power Saving");
		if (sysFsName == "power")
			return _("Power Saving");
		// Unknown name
		return sysFsName;
	};

	for (uint i = data.firstCoreIndex; i < data.firstCoreIndex + data.coreCount; i++) {
		char path[96];
		snprintf(path, 96,
		    "/sys/devices/system/cpu/cpu%u/cpufreq/"
		    "energy_performance_available_preferences",
		    i);

		auto sysFsNames = fileWords(path);
		if (sysFsNames.empty())
			continue;

		EnumerationVec enumVec;
		for (int i = 0; i < sysFsNames.size(); i++) {
			auto e = Enumeration{fromSysFsName(sysFsNames[i]), static_cast<unsigned>(i)};
			enumVec.push_back(e);
		}

		// Path of current value
		snprintf(path, 96,
		    "/sys/devices/system/cpu/cpu%u/cpufreq/energy_performance_preference", i);

		auto getFunc = [=]() -> std::optional<AssignmentArgument> {
			auto string = fileContents(path);
			if (!string.has_value())
				return std::nullopt;

			for (int i = 0; i < enumVec.size(); i++) {
				// Comparison fails with newline
				if (fplus::trim('\n', *string) == sysFsNames[i])
					return enumVec[i].key;
			}
			return std::nullopt;
		};

		auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
			if (!std::holds_alternative<uint>(a))
				return AssignmentError::InvalidType;

			auto arg = std::get<uint>(a);
			if (!hasEnum(arg, enumVec))
				return AssignmentError::OutOfRange;

			std::ofstream file{path};
			if (file.good() && file << sysFsNames[arg])
				return std::nullopt;
			return AssignmentError::UnknownError;
		};

		Assignable a{setFunc, enumVec, getFunc, std::nullopt};

		char idStr[64];
		snprintf(idStr, 64, "%sCore%uEPP", data.identifier.c_str(), i);
		char name[32];
		snprintf(name, 32, "%s %u", _("Core"), i);

		if (getFunc().has_value()) {
			DeviceNode node{
			    .name = name,
			    .interface = a,
			    .hash = md5(idStr),
			};
			retval.push_back(node);
		}
	}
	return retval;
}

std::optional<Range<int>> cpuFreqRange(CPUData data) {
	// The proper limits seem to be at least in the last core index, the first two cores
	// report lower max speed, even though they can boost to the same frequency. WTF?
	// TODO: check all indices if this is wrong on some other system
	uint lastIndex = data.firstCoreIndex + data.coreCount - 1;
	char path[96];
	snprintf(path, 96, "/sys/devices/system/cpu/cpu%u/cpufreq/cpuinfo_min_freq", lastIndex);
	auto cpuInfoMinStr = fileContents(path);

	if (!cpuInfoMinStr.has_value())
		return std::nullopt;

	auto cpuInfoMin = std::stoi(*cpuInfoMinStr);

	snprintf(path, 96, "/sys/devices/system/cpu/cpu%u/cpufreq/cpuinfo_max_freq", lastIndex);
	auto cpuInfoMaxStr = fileContents(path);

	if (!cpuInfoMinStr.has_value())
		return std::nullopt;

	auto cpuInfoMax = std::stoi(*cpuInfoMaxStr);
	// kHz -> MHz
	return Range{cpuInfoMin / 1000, cpuInfoMax / 1000};
}

// Take the path format as argument since that's the only difference between min/max
std::vector<Assignable> freqLimitAssignableFromFormat(CPUData data, const char *format) {
	std::vector<Assignable> retval;
	auto range = cpuFreqRange(data);
	if (!range.has_value())
		return {};

	for (uint i = data.firstCoreIndex; i < data.firstCoreIndex + data.coreCount; i++) {
		char path[96];
		snprintf(path, 96, format, i);

		auto getFunc = [=]() -> std::optional<AssignmentArgument> {
			auto contents = fileContents(path);
			if (!contents.has_value())
				return std::nullopt;
			return std::stoi(*contents) / 1000;
		};

		auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
			if (!std::holds_alternative<int>(a))
				return AssignmentError::InvalidType;

			auto arg = std::get<int>(a);
			if (arg < range->min || arg > range->max)
				return AssignmentError::OutOfRange;

			std::ofstream file{path};
			// MHz -> kHz
			if (file << (arg * 1000))
				return std::nullopt;
			return AssignmentError::UnknownError;
		};
		Assignable a{setFunc, *range, getFunc, _("MHz")};
		retval.push_back(a);
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getGovernorMinimums(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;
	auto format = "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_min_freq";
	auto assignables = freqLimitAssignableFromFormat(data, format);

	for (uint i = 0; i < assignables.size(); i++) {
		char idStr[64];
		snprintf(idStr, 64, "%sCore%uGovernorMin", data.identifier.c_str(), i);
		char nameStr[32];
		snprintf(nameStr, 32, "%s %u", _("Core"), i);

		DeviceNode node{
		    .name = nameStr,
		    .interface = assignables[i],
		    .hash = md5(idStr),
		};
		retval.push_back(node);
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getGovernorMaximums(CPUData data) {
	std::vector<TreeNode<DeviceNode>> retval;
	auto format = "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_max_freq";
	auto assignables = freqLimitAssignableFromFormat(data, format);

	for (uint i = 0; i < assignables.size(); i++) {
		char idStr[64];
		snprintf(idStr, 64, "%sCore%uGovernorMax", data.identifier.c_str(), i);
		char nameStr[32];
		snprintf(nameStr, 32, "%s %u", _("Core"), i);

		DeviceNode node{
		    .name = nameStr,
		    .interface = assignables[i],
		    .hash = md5(idStr),
		};
		retval.push_back(node);
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getCoreVoltage(CPUData data) {
	if (data.vendorId != "GenuineIntel")
		return {};

	auto func = [=]() -> ReadResult {
		// Increment in volts
		const double factor = 1 / pow(2, 13);
		// MSR_PERF_STATUS, bits 31:47
		auto value = readMsr(0x198, 0xffff00000000, data.firstCoreIndex);
		if (!value.has_value() || *value == 0)
			return ReadError::UnknownError;

		auto increment = *value >> 32;
		// V -> mV
		return (increment * factor) * 1000;
	};
	if (!hasReadableValue(func()))
		return {};
	// Normally this is shown in V, but we already have translations for mV
	return {DeviceNode{
	    .name = _("Core Voltage"),
	    .interface = DynamicReadable{func, _("mV")},
	    .hash = md5(data.identifier + "Core Voltage"),
	}};
}

std::vector<TreeNode<DeviceNode>> getVoltageRoot(CPUData data) {
	// This seems to do something on Intel too, even when EPB is available
	return {DeviceNode{
	    .name = _("Voltages"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Voltage Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getEPPRoot(CPUData data) {
	// This seems to do something on Intel too, even when EPB is available
	return {DeviceNode{
	    .name = _("Power Usage Mode"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "EPP Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getGovernorMaximumsRoot(CPUData data) {
	// Scaling governor root, eg. powersave, performance
	return {DeviceNode{
	    .name = _("Maximum Frequencies"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Governor Maximums Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getGovernorMinimumsRoot(CPUData data) {
	// Scaling governor root, eg. powersave, performance
	return {DeviceNode{
	    .name = _("Minimum Frequencies"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Governor Minimums Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCPUGovernorRoot(CPUData data) {
	// Scaling governor root, eg. powersave, performance
	return {DeviceNode{
	    .name = _("Governor"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Scaling Governor Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getGovernorRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Governor"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Governor Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getIntelEPBRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Power Saving Tendencies"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "EPB"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFreqsRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Frequencies"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Frequencies"),
	}};
}

std::vector<TreeNode<DeviceNode>> getUtilizationsRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Utilizations"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Utilizations"),
	}};
}

std::vector<TreeNode<DeviceNode>> getTemperaturesRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Temperatures"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Temperatures"),
	}};
}

std::vector<TreeNode<DeviceNode>> getPowerRoot(CPUData data) {
	return {DeviceNode{
	    .name = _("Power"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Power Root"),
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
		}},
		{getUtilizationsRoot, {
			{getUtilizations, {}}
		}},
		{getIntelEPBRoot, {
			{getIntelEPBNodes, {}}
		}},
		{getEPPRoot, {
			{getEPPNodes, {}}
		}},
		{getGovernorRoot, {
			{getCPUGovernorRoot, {
				{getGovernors, {}},
			}},
			{getGovernorMinimumsRoot, {
				{getGovernorMinimums, {}},
			}},
			{getGovernorMaximumsRoot, {
				{getGovernorMaximums, {}}
			}}
		}},
		{getPowerRoot, {
			{getTotalPowerUsage, {}},
			{getDramPowerUsage, {}},
			{getCorePowerUsage, {}}
		}},
		{getVoltageRoot, {
			{getCoreVoltage, {}}
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
