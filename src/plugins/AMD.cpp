#include <Crypto.hpp>
#include <patterns.hpp>
#include <Plugin.hpp>
#include <TreeConstructor.hpp>
#include <Utils.hpp>

#include <cmath>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <fplus/fplus.hpp>
#include <functional>
#include <fstream>
#include <iostream>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <libintl.h>
#include <unistd.h>
#include <xf86drm.h>

#define _(String) gettext(String)

extern int errno;

#ifndef DRM_RENDER_MINOR_NAME
	#define DRM_RENDER_MINOR_NAME "renderD"
#endif

#define _HWMON_NAME "hwmon"
#define _AMDGPU_NAME "amdgpu"
#define DEVICE_FILE_PREFIX DRM_DIR_NAME "/" DRM_RENDER_MINOR_NAME
#define RENDERD_OFFSET 128

using namespace mpark::patterns;
using namespace TuxClocker::Plugin;
using namespace TuxClocker::Crypto;
using namespace TuxClocker::Device;
using namespace TuxClocker;

namespace fs = std::filesystem;

enum PPTableType {
	Vega10,
	Navi
};

struct VFPoint {
	int voltage;
	int clock;
};

struct AMDGPUData {
	// Full path, eg. /sys/class/drm/renderD128/device/hwmon
	std::string hwmonPath;
	amdgpu_device_handle devHandle;
	// Used as identifier
	std::string pciId;
	std::optional<PPTableType> ppTableType;
};

std::vector<std::string> pstateSectionLines(
    const std::string &header, const std::string &contents) {
	std::vector<std::string> retval;

	auto isNewline = [](char c) { return c == '\n'; };
	auto lines = fplus::split_by(isNewline, false, contents);

	int startIndex = -1;
	for (int i = 0; i < lines.size(); i++) {
		// Find section start
		if (lines[i].find(header) != std::string::npos) {
			startIndex = i + 1;
			break;
		}
	}
	if (startIndex == -1)
		return {};

	for (int i = startIndex; i < lines.size(); i++) {
		if (isdigit(lines[i].at(0))) {
			// We're still in the section
			retval.push_back(lines[i]);
		} else
			// Line doesn't start with digit, another section has started
			break;
	}
	return retval;
}

std::optional<Range<int>> parsePstateRangeLine(std::string title, const std::string &contents) {
	// For example:
	// MCLK:     625Mhz        930Mhz
	auto isNewline = [](char c) { return c == '\n'; };
	auto lines = fplus::split_by(isNewline, false, contents);

	for (auto &line : lines) {
		if (line.rfind(title, 0) == 0) {
			// Line starts with title
			// Only split on whitespace
			auto words = fplus::split_one_of(std::string{" "}, false, line);
			if (words.size() >= 3)
				return Range<int>{std::stoi(words[1]), std::stoi(words[2])};
		}
	}
	return std::nullopt;
}

std::optional<Range<int>> parsePstateRangeLineWithRead(std::string title, AMDGPUData data) {
	auto contents = fileContents(data.hwmonPath + "/pp_od_clk_voltage");
	if (!contents.has_value())
		return std::nullopt;

	return parsePstateRangeLine(title, *contents);
}

std::optional<std::pair<int, int>> parseLineValuePair(const std::string &line) {
	auto words = fplus::split_one_of(std::string{" "}, false, line);

	if (words.size() >= 3)
		return std::pair{std::stoi(words[1]), std::stoi(words[2])};
	return std::nullopt;
}

std::optional<int> parseLineValue(const std::string &line) {
	auto words = fplus::split_one_of(std::string{" "}, false, line);

	if (words.size() >= 2)
		return std::stoi(words[1]);
	return std::nullopt;
}

std::optional<VFPoint> vfPoint(const std::string &section, int index, const std::string &table) {
	auto lines = pstateSectionLines(section, table);
	if (lines.empty() && lines.size() < index + 1)
		return std::nullopt;

	auto line = lines[index];
	auto valuePair = parseLineValuePair(line);
	if (valuePair.has_value())
		return VFPoint{
		    .voltage = valuePair->second,
		    .clock = valuePair->first,
		};
	return std::nullopt;
}

// Same as above, but read the file
std::optional<VFPoint> vfPointWithRead(const std::string &section, int index, AMDGPUData data) {
	auto contents = fileContents(data.hwmonPath + "/pp_od_clk_voltage");
	if (!contents.has_value())
		return std::nullopt;
	return vfPoint(section, index, *contents);
}

std::optional<PPTableType> fromPPTableContents(const std::string &contents) {
	auto clockSection = pstateSectionLines("OD_SCLK", contents);
	if (!clockSection.empty()) {
		// Vega 10 has the voltage-frequency curve labeled OD_SCLK
		if (parseLineValuePair(clockSection.front()).has_value())
			return Vega10;
		// On Vega 20 it's a section of single values
		if (parseLineValue(clockSection.front()).has_value()) {
			auto first = parsePstateRangeLine("VDDC_CURVE_VOLT[0]", contents);
			auto fourth = parsePstateRangeLine("VDDC_CURVE_VOLT[3]", contents);

			// Navi (NV1X?) has three frequency-voltage points
			if (first.has_value() && !fourth.has_value())
				return Navi;
		}
	}
	return std::nullopt;
}

std::optional<AMDGPUData> fromRenderDFile(const fs::directory_entry &entry) {
	auto fd = open(entry.path().c_str(), O_RDONLY);
	auto v_ptr = drmGetVersion(fd);
	amdgpu_device_handle dev;
	uint32_t m, n;
	int devInitRetval = amdgpu_device_initialize(fd, &m, &n, &dev);
	if (fd > 0 && v_ptr && devInitRetval == 0 &&
	    std::string(v_ptr->name).find(_AMDGPU_NAME) != std::string::npos) {
		// Device uses amdgpu
		// Find hwmon path
		std::ostringstream stream;
		// Eg. renderD128
		auto filename = entry.path().filename().string();
		stream << "/sys/class/drm/" << filename << "/device/hwmon";

		std::optional<std::string> hwmonPath = std::nullopt;
		try {
			for (const auto &entry : fs::directory_iterator(stream.str())) {
				if (entry.path().filename().string().find("hwmon") !=
				    std::string::npos) {
					hwmonPath = entry.path().string();
					break;
				}
			}
		} catch (fs::filesystem_error &e) {
			goto fail;
		}
		if (!hwmonPath.has_value())
			goto fail;

		// Get PCI id
		drm_amdgpu_info_device info;
		if (amdgpu_query_info(dev, AMDGPU_INFO_DEV_INFO, sizeof(info), &info) != 0)
			goto fail;

		// Try to get powerplay table type
		std::optional<PPTableType> tableType = std::nullopt;
		auto contents = fileContents(*hwmonPath + "/pp_od_clk_voltage");
		if (contents.has_value())
			tableType = fromPPTableContents(*contents);

		drmFreeVersion(v_ptr);
		return AMDGPUData{
		    .hwmonPath = hwmonPath.value(),
		    .devHandle = dev,
		    .pciId = std::to_string(info.device_id),
		    .ppTableType = tableType,
		};
	}
fail:
	close(fd);
	drmFreeVersion(v_ptr);
	return std::nullopt;
}

std::vector<AMDGPUData> fromFilesystem() {
	std::vector<AMDGPUData> retval;
	// Iterate through files in GPU device folder and find which ones have amdgpu loaded
	for (const auto &entry : fs::directory_iterator(DRM_DIR_NAME)) {
		// Check if path contains 'renderD' so we don't create root nodes for 'cardX' too
		if (entry.path().string().find(DRM_RENDER_MINOR_NAME) != std::string::npos) {
			auto data = fromRenderDFile(entry);
			if (data.has_value())
				retval.push_back(data.value());
		}
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getTemperature(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		uint temp;
		// Always uses uintptr_t to write return data
		if (amdgpu_query_sensor_info(
			data.devHandle, AMDGPU_INFO_SENSOR_GPU_TEMP, sizeof(temp), &temp) == 0)
			return temp / 1000;
		return ReadError::UnknownError;
	};

	DynamicReadable dr{func, _("°C")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Temperature"),
		    .interface = dr,
		    .hash = md5(data.pciId + "Temperature"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getFanMode(AMDGPUData data) {
	char path[96];
	snprintf(path, 96, "%s/pwm1_enable", data.hwmonPath.c_str());
	if (!std::ifstream{path}.good())
		return {};

	// TODO: does everything correctly handle enumerations that don't start at zero?
	EnumerationVec enumVec{{_("Manual"), 1}, {_("Automatic"), 2}};

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto string = fileContents(path);
		if (!string.has_value())
			return std::nullopt;

		// We only handle automatic, see below
		auto value = static_cast<uint>(std::stoi(*string));
		if (value != 2)
			return std::nullopt;
		return 2;
	};

	// TODO: this may be wrong
	// Seems that fan speed is writable even with automatic mode, but writing '2'
	// without writing fan speed afterwards seems to reset to automatic
	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<uint>(a))
			return AssignmentError::InvalidType;

		auto value = std::get<uint>(a);
		if (!hasEnum(value, enumVec))
			return AssignmentError::OutOfRange;

		if (std::ofstream{path} << "2")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	Assignable a{setFunc, enumVec, getFunc, std::nullopt};

	return {DeviceNode{
	    .name = _("Fan Mode"),
	    .interface = a,
	    .hash = md5(data.pciId + "Fan Mode"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedWrite(AMDGPUData data) {
	char path[96];
	snprintf(path, 96, "%s/pwm1", data.hwmonPath.c_str());
	if (!std::ifstream{path}.good())
		return {};

	Range<int> range{0, 100};

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto string = fileContents(path);
		if (!string.has_value())
			return std::nullopt;

		double ratio = static_cast<double>(std::stoi(*string)) / 255;
		// ratio -> %
		return std::round((ratio * 100));
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto value = std::get<int>(a);
		if (value < range.min || value > range.max)
			return AssignmentError::OutOfRange;

		// % -> PWM value (0-255)
		auto ratio = static_cast<double>(value) / 100;
		uint target = std::floor(ratio * 255);
		if (std::ofstream{path} << target)
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	Assignable a{setFunc, range, getFunc, _("%")};

	return {DeviceNode{
	    .name = _("Fan Speed"),
	    .interface = a,
	    .hash = md5(data.pciId + "Fan Speed Write"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedRead(AMDGPUData data) {
	char path[96];

	snprintf(path, 96, "%s/fan1_max", data.hwmonPath.c_str());
	auto contents = fileContents(path);
	if (!contents.has_value())
		return {};

	int maxRPM = std::stoi(*contents);

	snprintf(path, 96, "%s/fan1_input", data.hwmonPath.c_str());

	auto func = [=]() -> ReadResult {
		auto string = fileContents(path);
		if (!string.has_value())
			return ReadError::UnknownError;

		int value = std::stoi(*string);
		double ratio = static_cast<double>(value) / static_cast<double>(maxRPM);
		return std::round(ratio * 100);
	};

	DynamicReadable dr{func, _("%")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Fan Speed"),
		    .interface = dr,
		    .hash = md5(data.pciId + "Fan Speed Read"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getPowerLimit(AMDGPUData data) {
	// Get delta of min and max fan RPMs
	char path[96];
	snprintf(path, 96, "%s/power1_cap_min", data.hwmonPath.c_str());
	auto contents = fileContents(path);
	if (!contents.has_value())
		return {};

	// uW -> W
	double minLimit = static_cast<double>(std::stoi(*contents)) / 1000000;

	snprintf(path, 96, "%s/power1_cap_max", data.hwmonPath.c_str());
	contents = fileContents(path);
	if (!contents.has_value())
		return {};

	double maxLimit = static_cast<double>(std::stoi(*contents)) / 1000000;
	Range<double> range{minLimit, maxLimit};

	snprintf(path, 96, "%s/power1_cap", data.hwmonPath.c_str());

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto string = fileContents(path);
		if (!string.has_value())
			return std::nullopt;

		int cur_uW = std::stoi(*string);
		return static_cast<double>(cur_uW) / 1000000;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<double>(a))
			return AssignmentError::InvalidType;

		auto value = std::get<double>(a);
		if (value < range.min || value > range.max)
			return AssignmentError::OutOfRange;

		// W -> uW
		auto target = std::round(value * 1000000);
		if (std::ofstream{path} << target)
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	Assignable a{setFunc, range, getFunc, _("W")};

	return {DeviceNode{
	    .name = _("Power Limit"),
	    .interface = a,
	    .hash = md5(data.pciId + "Power Limit"),
	}};
}

std::vector<TreeNode<DeviceNode>> getPowerUsage(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		uint power;
		if (amdgpu_query_sensor_info(data.devHandle, AMDGPU_INFO_SENSOR_GPU_AVG_POWER,
			sizeof(power), &power) == 0)
			return power;
		return ReadError::UnknownError;
	};

	DynamicReadable dr{func, _("W")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Power Usage"),
		    .interface = dr,
		    .hash = md5(data.pciId + "Power Usage"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getCoreClockRead(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		uint clock;
		if (amdgpu_query_sensor_info(
			data.devHandle, AMDGPU_INFO_SENSOR_GFX_SCLK, sizeof(clock), &clock) == 0)
			return clock;
		return ReadError::UnknownError;
	};

	DynamicReadable dr{func, _("MHz")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Core Clock"),
		    .interface = dr,
		    .hash = md5(data.pciId + "Core Clock"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getMemoryClockRead(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		uint clock;
		// TODO: is this actually the clock speed or memory controller clock?
		if (amdgpu_query_sensor_info(
			data.devHandle, AMDGPU_INFO_SENSOR_GFX_MCLK, sizeof(clock), &clock) == 0)
			return clock;
		return ReadError::UnknownError;
	};

	DynamicReadable dr{func, _("MHz")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Memory Clock"),
		    .interface = dr,
		    .hash = md5(data.pciId + "Memory Clock"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getVoltFreqFreq(AMDGPUData data) {
	static amdgpu_device_handle latestDev = nullptr;
	static int pointId = 0;
	std::vector<TreeNode<DeviceNode>> retval = {};

	if (data.devHandle != latestDev)
		// Start from zero for new device
		pointId = 0;

	latestDev = data.devHandle;

	// TODO: assuming range is the same for all points
	auto range = parsePstateRangeLineWithRead("VDDC_CURVE_SCLK[0]", data);
	if (!range.has_value()) {
		pointId++;
		return {};
	}

	// Make a copy so the lambda keeps using the right id
	auto id = pointId;
	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto curvePoint = vfPointWithRead("OD_VDDC_CURVE", id, data);
		if (!curvePoint.has_value())
			return std::nullopt;

		return curvePoint->clock;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range->min || target > range->max)
			return AssignmentError::OutOfRange;

		auto curvePoint = vfPointWithRead("OD_VDDC_CURVE", id, data);
		if (!curvePoint.has_value())
			return AssignmentError::UnknownError;

		std::ofstream file{data.hwmonPath + "/pp_od_clk_voltage"};
		char cmdString[32];
		snprintf(cmdString, 32, "vc %i %i %i", id, target, curvePoint->voltage);

		if (file << cmdString && file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	Assignable a{setFunc, *range, getFunc, _("MHz")};
	pointId++;

	if (getFunc().has_value())
		return {DeviceNode{
		    .name = _("Core Clock"),
		    .interface = a,
		    .hash = md5(data.pciId + "VFClock" + std::to_string(id)),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getVoltFreqVolt(AMDGPUData data) {
	static amdgpu_device_handle latestDev = nullptr;
	static int pointId = 0;
	std::vector<TreeNode<DeviceNode>> retval = {};

	if (data.devHandle != latestDev)
		// Start from zero for new device
		pointId = 0;

	latestDev = data.devHandle;

	// TODO: assuming range is the same for all points
	auto range = parsePstateRangeLineWithRead("VDDC_CURVE_VOLT[0]", data);
	if (!range.has_value()) {
		pointId++;
		return {};
	}

	// Make a copy so the lambda keeps using the right id
	auto id = pointId;
	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto curvePoint = vfPointWithRead("OD_VDDC_CURVE", id, data);
		if (!curvePoint.has_value())
			return std::nullopt;

		return curvePoint->voltage;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range->min || target > range->max)
			return AssignmentError::OutOfRange;

		auto curvePoint = vfPointWithRead("OD_VDDC_CURVE", id, data);
		if (!curvePoint.has_value())
			return AssignmentError::UnknownError;

		std::ofstream file{data.hwmonPath + "/pp_od_clk_voltage"};
		char cmdString[32];
		snprintf(cmdString, 32, "vc %i %i %i", id, curvePoint->clock, target);

		if (file << cmdString && file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	Assignable a{setFunc, *range, getFunc, _("mV")};
	pointId++;

	if (getFunc().has_value())
		return {DeviceNode{
		    .name = _("Core Voltage"),
		    .interface = a,
		    .hash = md5(data.pciId + "VFVoltage" + std::to_string(id)),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getVoltFreqNodes(AMDGPUData data) {
	// Root item for voltage and frequency of a point
	std::vector<TreeNode<DeviceNode>> retval;
	if (!data.ppTableType.has_value() || *data.ppTableType != Navi)
		return {};

	auto path = data.hwmonPath + "/pp_od_clk_voltage";
	auto tableContents = fileContents(path);
	if (!tableContents.has_value())
		return {};

	auto lines = pstateSectionLines("OD_VDDC_CURVE", *tableContents);
	char name[32];
	for (int i = 0; i < lines.size(); i++) {
		snprintf(name, 32, "%s %i", _("Point"), i);

		DeviceNode node{
		    .name = name,
		    .interface = std::nullopt,
		    .hash = md5(data.pciId + "VFPoint" + std::to_string(i)),
		};
		retval.push_back(node);
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getVoltageRead(AMDGPUData data) {
	auto func = [=](int sensorType) -> ReadResult {
		uint volt;
		if (amdgpu_query_sensor_info(data.devHandle, sensorType, sizeof(volt), &volt) == 0)
			return volt;
		return ReadError::UnknownError;
	};
	// Try to get northbridge voltage if graphics voltage can't be fetched
	std::optional<int> sensorType;
	if (hasReadableValue(func(AMDGPU_INFO_SENSOR_VDDGFX)))
		sensorType = AMDGPU_INFO_SENSOR_VDDGFX;
	if (hasReadableValue(func(AMDGPU_INFO_SENSOR_VDDNB)))
		sensorType = AMDGPU_INFO_SENSOR_VDDNB;

	if (!sensorType.has_value())
		return {};

	auto funcWithSensorType = [=]() -> ReadResult { return func(*sensorType); };
	DynamicReadable dr{funcWithSensorType, _("mV")};

	return {DeviceNode{
	    .name = _("Core Voltage"),
	    .interface = dr,
	    .hash = md5(data.pciId + "Core Voltage"),
	}};
}

std::vector<TreeNode<DeviceNode>> getForcePerfLevel(AMDGPUData data) {
	// Performance parameter control
	std::array<std::string, 8> sysFsNames = {"auto", "low", "high", "manual",
	    "profile_standard", "profile_min_sclk", "profile_min_mclk", "profile_peak"};

	EnumerationVec enumVec = {{_("Automatic"), 0}, {_("Lowest"), 1}, {_("Highest"), 2},
	    {_("Manual"), 3}, {_("Base Levels"), 4}, {_("Lowest Core Clock"), 5},
	    {_("Lowest Memory Clock"), 6}, {_("Highest Clocks"), 7}};

	auto path = data.hwmonPath + "/power_dpm_force_performance_level";

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto string = fileContents(path);
		if (!string.has_value())
			return std::nullopt;

		for (int i = 0; i < enumVec.size(); i++) {
			if (string->find(sysFsNames[i]) != std::string::npos)
				return enumVec[i].key;
		}
		return std::nullopt;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		std::ofstream file{path};
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

	if (getFunc().has_value())
		return {DeviceNode{
		    .name = _("Performance Parameter Control"),
		    .interface = std::nullopt,
		    .hash = md5(data.pciId + "Performance Parameter Control"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getVoltFreqRoot(AMDGPUData data) {
	if (data.ppTableType.has_value() && *data.ppTableType == Navi)
		return {DeviceNode{
		    .name = _("Voltage-Frequency Curve"),
		    .interface = std::nullopt,
		    .hash = md5(data.pciId + "Voltage-Frequency Curve"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getClocksRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Clocks"),
	    .interface = std::nullopt,
	    .hash = md5(data.pciId + "Clocks"),
	}};
}

std::vector<TreeNode<DeviceNode>> getPerformanceRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Performance"),
	    .interface = std::nullopt,
	    .hash = md5(data.pciId + "Performance"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Fans"),
	    .interface = std::nullopt,
	    .hash = md5(data.pciId + "Fans"),
	}};
}

std::vector<TreeNode<DeviceNode>> getPowerRoot(AMDGPUData data) {
	// Root for power usage and power limit
	return {DeviceNode{
	    .name = _("Power"),
	    .interface = std::nullopt,
	    .hash = md5(data.pciId + "Power"),
	}};
}

std::vector<TreeNode<DeviceNode>> getGPUName(AMDGPUData data) {
	auto name = amdgpu_get_marketing_name(data.devHandle);
	if (name) {
		return {DeviceNode{
		    .name = name,
		    .interface = std::nullopt,
		    .hash = md5(data.pciId),
		}};
	}
	return {};
}

// clang-format off
auto gpuTree = TreeConstructor<AMDGPUData, DeviceNode>{
	getGPUName, {
		{getTemperature, {}},
		{getFanRoot, {
			{getFanMode, {}},
			{getFanSpeedWrite, {}},
			{getFanSpeedRead, {}}
		}},
		{getPowerRoot, {
			{getPowerLimit, {}},
			{getPowerUsage, {}}
		}},
		{getPerformanceRoot, {
			{getClocksRoot, {
				{getMemoryClockRead, {}},
				{getCoreClockRead, {}}
			}},
			{getVoltageRead, {}},
			{getForcePerfLevel, {}},
			{getVoltFreqRoot, {
				{getVoltFreqNodes, {
					{getVoltFreqFreq, {}},
					{getVoltFreqVolt, {}}
				}}
			}}
		}}
	}
};
// clang-format on

class AMDPlugin : public DevicePlugin {
public:
	std::optional<InitializationError> initializationError() { return std::nullopt; }
	TreeNode<DeviceNode> deviceRootNode();
	~AMDPlugin();
private:
	std::vector<AMDGPUData> m_gpuDataVec;
};

TreeNode<DeviceNode> AMDPlugin::deviceRootNode() {
	TreeNode<DeviceNode> root;

	auto dataVec = fromFilesystem();
	m_gpuDataVec = dataVec;

	for (auto &data : dataVec)
		constructTree(gpuTree, root, data);

	return root;
}

AMDPlugin::~AMDPlugin() {
	for (auto info : m_gpuDataVec) {
		amdgpu_device_deinitialize(info.devHandle);
	}
}

TUXCLOCKER_PLUGIN_EXPORT(AMDPlugin)
