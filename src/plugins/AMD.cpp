#include <AMDUtils.hpp>
#include <Crypto.hpp>
#include <Plugin.hpp>
#include <TreeConstructor.hpp>
#include <Utils.hpp>

#include <cmath>
#include <errno.h>
#include <fplus/fplus.hpp>
#include <fstream>
#include <iostream>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <libintl.h>

#ifdef WITH_HWDATA
	#include <HWData.hpp>
#endif

#define _(String) gettext(String)

extern int errno;

using namespace TuxClocker::Plugin;
using namespace TuxClocker::Crypto;
using namespace TuxClocker::Device;
using namespace TuxClocker;

using AssignmentFunction = std::function<std::optional<AssignmentError>(AssignmentArgument)>;

enum VoltFreqType {
	MemoryPState,
	CorePState,
	CoreVFCurve
};

// Minimum (where available) and max clocks
enum SingleAssignableType {
	CoreClock,
	MemoryClock,
};

EnumerationVec performanceLevelEnumVec = {{_("Automatic"), 0}, {_("Lowest"), 1}, {_("Highest"), 2},
    {_("Manual"), 3}, {_("Base Levels"), 4}, {_("Lowest Core Clock"), 5},
    {_("Lowest Memory Clock"), 6}, {_("Highest Clocks"), 7}};

std::array<std::string, 8> performanceLevelSysFsNames = {"auto", "low", "high", "manual",
    "profile_standard", "profile_min_sclk", "profile_min_mclk", "profile_peak"};

// Separate function so we can set this to manual when writing to pp_od_clk_voltage
std::optional<AssignmentError> setPerformanceLevel(AssignmentArgument a, AMDGPUData data) {
	std::array<std::string, 8> sysFsNames = {"auto", "low", "high", "manual",
	    "profile_standard", "profile_min_sclk", "profile_min_mclk", "profile_peak"};

	auto path = data.devPath + "/power_dpm_force_performance_level";
	std::ofstream file{path};
	if (!file.good())
		return AssignmentError::UnknownError;

	if (!std::holds_alternative<uint>(a))
		return AssignmentError::InvalidType;

	auto arg = std::get<uint>(a);
	if (!hasEnum(arg, performanceLevelEnumVec))
		return AssignmentError::OutOfRange;

	if (file << sysFsNames[arg])
		return std::nullopt;

	return AssignmentError::UnknownError;
};

// Implicitly set performance level to manual when writing to pp_od_clk_voltage
std::optional<AssignmentError> withManualPerformanceLevel(
    const AssignmentFunction &func, AssignmentArgument a, AMDGPUData data) {
	// Manual
	auto retval = setPerformanceLevel(3u, data);
	if (retval.has_value())
		// Error occurred
		return retval;

	return func(a);
}

// Shared function to get memory and core clock pstate assignables
std::optional<Assignable> vfPointClockAssignable(
    VoltFreqType vfType, uint pointIndex, Range<int> range, AMDGPUData data) {
	// Eg. the 's' in s 0 400 500
	const char *typeString;
	const char *sectionHeader;

	switch (vfType) {
	case MemoryPState: {
		typeString = "m";
		sectionHeader = "OD_MCLK";
		break;
	}
	case CorePState: {
		typeString = "s";
		sectionHeader = "OD_SCLK";
		break;
	}
	case CoreVFCurve: {
		typeString = "vc";
		sectionHeader = "OD_VDDC_CURVE";
		break;
	}
	}

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto vfPoint = vfPointWithRead(sectionHeader, pointIndex, data);
		if (!vfPoint.has_value())
			return std::nullopt;

		// Memory clock -> controller clock for memory clocks
		if (vfType == MemoryPState)
			return toMemoryClock(vfPoint->clock, data);

		return vfPoint->clock;
	};

	if (!getFunc().has_value())
		return std::nullopt;

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		auto vfPoint = vfPointWithRead(sectionHeader, pointIndex, data);
		if (!vfPoint.has_value())
			return AssignmentError::UnknownError;

		// Memory clock -> controller clock for memory clocks
		if (vfType == MemoryPState)
			target = toControllerClock(target, data);

		std::ofstream file{data.devPath + "/pp_od_clk_voltage"};
		char cmdString[32];
		snprintf(
		    cmdString, 32, "%s %u %i %i", typeString, pointIndex, target, vfPoint->voltage);

		if (file << cmdString && file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	auto setWithPerfLevel = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		return withManualPerformanceLevel(setFunc, a, data);
	};

	return Assignable{setWithPerfLevel, range, getFunc, _("MHz")};
}

// Same as above, but for editing single values like min/max clocks
std::optional<Assignable> singleValueAssignable(SingleAssignableType type, uint pointIndex,
    Range<int> range, std::string unit, AMDGPUData data) {
	auto path = data.devPath + "/pp_od_clk_voltage";
	auto contents = fileContents(path);
	if (!contents.has_value())
		return {};

	const char *typeString;
	const char *sectionHeader;

	switch (type) {
	case MemoryClock: {
		typeString = "m";
		sectionHeader = "OD_MCLK";
		break;
	}
	case CoreClock: {
		typeString = "s";
		sectionHeader = "OD_SCLK";
		break;
	}
	}

	auto lines = pstateSectionLines(sectionHeader, *contents);
	if (lines.size() < pointIndex + 1)
		return std::nullopt;

	// The index in sysfs can be '1' even if there's only one point
	auto words = fplus::split_words(false, lines[pointIndex]);
	int sysFsIndex = std::stoi(words.front());

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto contents = fileContents(path);
		if (!contents.has_value())
			return std::nullopt;

		auto lines = pstateSectionLines(sectionHeader, *contents);
		if (lines.size() < pointIndex + 1)
			return std::nullopt;

		auto value = parseLineValue(lines[pointIndex]);
		if (!value.has_value())
			return std::nullopt;

		// Memory clock -> controller clock for memory clocks
		if (type == MemoryClock)
			return toMemoryClock(*value, data);

		return *value;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		// Memory clock -> controller clock for memory clocks
		if (type == MemoryClock)
			target = toControllerClock(target, data);

		std::ofstream file{data.devPath + "/pp_od_clk_voltage"};
		char cmdString[32];
		snprintf(cmdString, 32, "%s %i %i", typeString, sysFsIndex, target);

		if (file << cmdString && file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	auto setWithPerfLevel = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		return withManualPerformanceLevel(setFunc, a, data);
	};

	return Assignable{setWithPerfLevel, range, getFunc, unit};
}

std::optional<Assignable> vfPointVoltageAssignable(
    VoltFreqType vfType, uint pointIndex, Range<int> range, AMDGPUData data) {
	// Eg. the 's' in s 0 400 500
	const char *typeString;
	const char *sectionHeader;

	switch (vfType) {
	case MemoryPState: {
		typeString = "m";
		sectionHeader = "OD_MCLK";
		break;
	}
	case CorePState: {
		typeString = "s";
		sectionHeader = "OD_SCLK";
		break;
	}
	case CoreVFCurve: {
		typeString = "vc";
		sectionHeader = "OD_VDDC_CURVE";
		break;
	}
	}

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto vfPoint = vfPointWithRead(sectionHeader, pointIndex, data);
		if (!vfPoint.has_value())
			return std::nullopt;

		return vfPoint->voltage;
	};

	if (!getFunc().has_value())
		return std::nullopt;

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		auto vfPoint = vfPointWithRead(sectionHeader, pointIndex, data);
		if (!vfPoint.has_value())
			return AssignmentError::UnknownError;

		std::ofstream file{data.devPath + "/pp_od_clk_voltage"};
		char cmdString[32];
		snprintf(
		    cmdString, 32, "%s %u %i %i", typeString, pointIndex, vfPoint->clock, target);

		if (file << cmdString && file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	auto setWithPerfLevel = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		return withManualPerformanceLevel(setFunc, a, data);
	};

	return Assignable{setWithPerfLevel, range, getFunc, _("mV")};
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
		    .hash = md5(data.identifier + "Temperature"),
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
	    .hash = md5(data.identifier + "Fan Mode"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedWrite(AMDGPUData data) {
	// Don't try to use pre-6.7 interface on RX 7000
	// TODO: potential regression, if this exists on older cards, but doesn't function
	char fanCurvePath[128];
	snprintf(fanCurvePath, 128, "%s/gpu_od/fan_ctrl/fan_curve", data.devPath.c_str());
	if (std::ifstream{fanCurvePath}.good())
		return {};

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
	    .hash = md5(data.identifier + "Fan Speed Write"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedWriteRX7000(AMDGPUData data) {
	char fanCurvePath[128];
	snprintf(fanCurvePath, 128, "%s/gpu_od/fan_ctrl/fan_curve", data.devPath.c_str());
	if (!std::ifstream{fanCurvePath}.good())
		return {};

	auto contents = fileContents(fanCurvePath);
	if (!contents.has_value())
		return {};

	// We don't care acout the temp range, since we set all points to desired speed
	auto speedRange = fromFanCurveContents(*contents);
	if (!speedRange.has_value())
		return {};

	// Only fetch temperatures once
	auto temps = fanCurveTempsFromContents(*contents);
	if (temps.empty())
		return {};

	// Doesn't make sense with what we do
	auto getFunc = [] { return std::nullopt; };

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (speedRange->min > target || speedRange->max < target)
			return AssignmentError::OutOfRange;

		// Write all curve points to same value
		std::ofstream file{fanCurvePath};
		for (int i = 0; i < temps.size(); i++) {
			char cmdString[32];
			// TODO: docs say PWM but internet says percentage
			snprintf(cmdString, 32, "%i %i %i", i, temps[i], target);
			if (!(file << cmdString))
				return AssignmentError::UnknownError;
		}

		if (file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	return {DeviceNode{
	    .name = _("Fan Speed"),
	    .interface = Assignable{setFunc, *speedRange, getFunc, _("%")},
	    .hash = md5(data.identifier + "RX7000 Fan Speed"),
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
		    .hash = md5(data.identifier + "Fan Speed Read"),
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
	    .hash = md5(data.identifier + "Power Limit"),
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
		    .hash = md5(data.identifier + "Power Usage"),
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
		    .hash = md5(data.identifier + "Core Clock"),
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
		    .hash = md5(data.identifier + "Memory Clock"),
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
	pointId++;

	auto assignable = vfPointClockAssignable(CoreVFCurve, id, *range, data);
	if (!assignable.has_value())
		return {};

	// The rest of this code should work the same on Navi and RDNA 3
	auto name = (*data.ppTableType == Navi) ? _("Core Clock") : _("Core Clock Offset");

	return {DeviceNode{
	    .name = name,
	    .interface = *assignable,
	    .hash = md5(data.identifier + "VFClock" + std::to_string(id)),
	}};
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
	pointId++;

	auto assignable = vfPointVoltageAssignable(CoreVFCurve, id, *range, data);
	if (!assignable.has_value())
		return {};

	// The rest of this code should work the same on Navi and RDNA 3
	auto name = (*data.ppTableType == Navi) ? _("Core Voltage") : _("Core Voltage Offset");

	return {DeviceNode{
	    .name = name,
	    .interface = *assignable,
	    .hash = md5(data.identifier + "VFVoltage" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getCorePStateFreq(AMDGPUData data) {
	static amdgpu_device_handle latestDev = nullptr;
	static int pointId = 0;
	std::vector<TreeNode<DeviceNode>> retval = {};

	if (data.devHandle != latestDev)
		// Start from zero for new device
		pointId = 0;

	latestDev = data.devHandle;

	auto range = parsePstateRangeLineWithRead("SCLK", data);
	if (!range.has_value()) {
		pointId++;
		return {};
	}

	// Make a copy so the lambda keeps using the right id
	auto id = pointId;
	auto assignable = vfPointClockAssignable(CorePState, id, *range, data);

	pointId++;

	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Core Clock"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "CorePStateFreq" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getCorePStateVolt(AMDGPUData data) {
	static amdgpu_device_handle latestDev = nullptr;
	static int pointId = 0;
	std::vector<TreeNode<DeviceNode>> retval = {};

	if (data.devHandle != latestDev)
		// Start from zero for new device
		pointId = 0;

	latestDev = data.devHandle;

	auto range = parsePstateRangeLineWithRead("VDDC", data);
	if (!range.has_value()) {
		pointId++;
		return {};
	}

	// Make a copy so the lambda keeps using the right id
	auto id = pointId;
	auto assignable = vfPointVoltageAssignable(CorePState, id, *range, data);

	pointId++;

	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Core Voltage"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "CorePStateVolt" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getMemoryPStateFreq(AMDGPUData data) {
	static amdgpu_device_handle latestDev = nullptr;
	static int pointId = 0;
	std::vector<TreeNode<DeviceNode>> retval = {};

	if (data.devHandle != latestDev)
		// Start from zero for new device
		pointId = 0;

	latestDev = data.devHandle;

	auto rangeController = parsePstateRangeLineWithRead("MCLK", data);
	if (!rangeController.has_value()) {
		pointId++;
		return {};
	}

	// Controller clock -> effective clock
	Range<int> range{
	    toMemoryClock(rangeController->min, data), toMemoryClock(rangeController->max, data)};

	// Make a copy so the lambda keeps using the right id
	auto id = pointId;
	// Conversion back to controller clock is handled there
	auto assignable = vfPointClockAssignable(MemoryPState, id, range, data);

	pointId++;

	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Memory Clock"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "MemoryPStateFreq" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getMemoryPStateVolt(AMDGPUData data) {
	static amdgpu_device_handle latestDev = nullptr;
	static int pointId = 0;
	std::vector<TreeNode<DeviceNode>> retval = {};

	if (data.devHandle != latestDev)
		// Start from zero for new device
		pointId = 0;

	latestDev = data.devHandle;

	auto range = parsePstateRangeLineWithRead("VDDC", data);
	if (!range.has_value()) {
		pointId++;
		return {};
	}

	// Make a copy so the lambda keeps using the right id
	auto id = pointId;
	auto assignable = vfPointVoltageAssignable(MemoryPState, id, *range, data);

	pointId++;

	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Memory Voltage"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "MemoryPStateVolt" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getVoltFreqNodes(AMDGPUData data) {
	// Root item for voltage and frequency of a point
	std::vector<TreeNode<DeviceNode>> retval;
	if (!data.ppTableType.has_value() &&
	    (*data.ppTableType != Navi && *data.ppTableType != SMU13))
		return {};

	auto path = data.devPath + "/pp_od_clk_voltage";
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
		    .hash = md5(data.identifier + "VFPoint" + std::to_string(i)),
		};
		retval.push_back(node);
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getCorePStateNodes(AMDGPUData data) {
	// Root item for voltage and frequency of a pstate
	std::vector<TreeNode<DeviceNode>> retval;
	if (!data.ppTableType.has_value() || *data.ppTableType != Vega10)
		return {};

	auto path = data.devPath + "/pp_od_clk_voltage";
	auto tableContents = fileContents(path);
	if (!tableContents.has_value())
		return {};

	auto lines = pstateSectionLines("OD_SCLK", *tableContents);
	char name[32];
	for (int i = 0; i < lines.size(); i++) {
		snprintf(name, 32, "%s %i", _("State"), i);

		DeviceNode node{
		    .name = name,
		    .interface = std::nullopt,
		    .hash = md5(data.identifier + "PState" + std::to_string(i)),
		};
		retval.push_back(node);
	}
	return retval;
}

std::vector<TreeNode<DeviceNode>> getMemoryPStateNodes(AMDGPUData data) {
	// Root item for voltage and frequency of a pstate
	std::vector<TreeNode<DeviceNode>> retval;
	if (!data.ppTableType.has_value() || *data.ppTableType != Vega10)
		return {};

	auto path = data.devPath + "/pp_od_clk_voltage";
	auto tableContents = fileContents(path);
	if (!tableContents.has_value())
		return {};

	auto lines = pstateSectionLines("OD_MCLK", *tableContents);
	char name[32];
	for (int i = 0; i < lines.size(); i++) {
		snprintf(name, 32, "%s %i", _("State"), i);

		DeviceNode node{
		    .name = name,
		    .interface = std::nullopt,
		    .hash = md5(data.identifier + "MemoryPState" + std::to_string(i)),
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
	    .hash = md5(data.identifier + "Core Voltage"),
	}};
}

std::vector<TreeNode<DeviceNode>> getForcePerfLevel(AMDGPUData data) {
	// Performance parameter control
	auto path = data.devPath + "/power_dpm_force_performance_level";

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto string = fileContents(path);
		if (!string.has_value())
			return std::nullopt;

		for (int i = 0; i < performanceLevelEnumVec.size(); i++) {
			if (string->find(performanceLevelSysFsNames[i]) != std::string::npos)
				return performanceLevelEnumVec[i].key;
		}
		return std::nullopt;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		return setPerformanceLevel(a, data);
	};

	Assignable a{setFunc, performanceLevelEnumVec, getFunc, std::nullopt};

	if (getFunc().has_value())
		return {DeviceNode{
		    .name = _("Performance Parameter Control"),
		    .interface = std::nullopt,
		    .hash = md5(data.identifier + "Performance Parameter Control"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getCoreUtilization(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		uint util;
		if (amdgpu_query_sensor_info(
			data.devHandle, AMDGPU_INFO_SENSOR_GPU_LOAD, sizeof(util), &util) == 0)
			return util;
		return ReadError::UnknownError;
	};

	DynamicReadable dr{func, _("%")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Core Utilization"),
		    .interface = dr,
		    .hash = md5(data.identifier + "Core Utilization"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getMemoryUtilization(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		auto string = fileContents(data.hwmonPath + "/mem_busy_percent");
		if (!string.has_value())
			return ReadError::UnknownError;
		return static_cast<uint>(std::stoi(*string));
	};

	DynamicReadable dr{func, _("%")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Memory Utilization"),
		    .interface = dr,
		    .hash = md5(data.identifier + "Memory Utilization"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getMaxMemoryClock(AMDGPUData data) {
	if (!data.ppTableType.has_value())
		return {};

	auto t = *data.ppTableType;
	if (t != Navi && t != SMU13 && t != Vega20Other)
		return {};

	// Per kernel documentation, if there's only one index for memory clock, it means maximum
	auto lines = pstateSectionLinesWithRead("OD_MCLK", data);
	if (lines.size() != 1 && lines.size() != 2)
		return {};

	// Index refers to nth line after 'OD_MCLK'
	auto index = (lines.size() == 1) ? 0 : 1;

	auto controllerRange = parsePstateRangeLineWithRead("MCLK", data);
	if (!controllerRange.has_value())
		return {};

	Range<int> range{
	    toMemoryClock(controllerRange->min, data), toMemoryClock(controllerRange->max, data)};

	auto assignable = singleValueAssignable(MemoryClock, index, range, _("MHz"), data);
	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Maximum Memory Clock"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "Maximum Memory Clock"),
	}};
}

std::vector<TreeNode<DeviceNode>> getMinMemoryClock(AMDGPUData data) {
	if (!data.ppTableType.has_value())
		return {};

	auto t = *data.ppTableType;
	if (t != Navi && t != SMU13 && t != Vega20Other)
		return {};

	// Per kernel documentation, two indices means there's a minimum clock
	auto lines = pstateSectionLinesWithRead("OD_MCLK", data);
	if (lines.size() != 2)
		return {};

	auto controllerRange = parsePstateRangeLineWithRead("MCLK", data);
	if (!controllerRange.has_value())
		return {};

	Range<int> range{
	    toMemoryClock(controllerRange->min, data), toMemoryClock(controllerRange->max, data)};

	auto assignable = singleValueAssignable(MemoryClock, 0, range, _("MHz"), data);
	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Minimum Memory Clock"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "Minimum Memory Clock"),
	}};
}

std::vector<TreeNode<DeviceNode>> getMaxCoreClock(AMDGPUData data) {
	if (!data.ppTableType.has_value())
		return {};

	auto t = *data.ppTableType;
	if (t != Navi && t != SMU13 && t != Vega20Other)
		return {};

	auto lines = pstateSectionLinesWithRead("OD_SCLK", data);
	if (lines.size() != 2)
		return {};

	auto range = parsePstateRangeLineWithRead("SCLK", data);
	if (!range.has_value())
		return {};

	// Second index is max
	auto assignable = singleValueAssignable(CoreClock, 1, *range, _("MHz"), data);
	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Maximum Core Clock"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "Maximum Core Clock"),
	}};
}

std::vector<TreeNode<DeviceNode>> getMinCoreClock(AMDGPUData data) {
	if (!data.ppTableType.has_value())
		return {};

	auto t = *data.ppTableType;
	if (t != Navi && t != SMU13 && t != Vega20Other)
		return {};

	auto lines = pstateSectionLinesWithRead("OD_SCLK", data);
	if (lines.size() != 2)
		return {};

	auto range = parsePstateRangeLineWithRead("SCLK", data);
	if (!range.has_value())
		return {};

	// First index is min
	auto assignable = singleValueAssignable(CoreClock, 0, *range, _("MHz"), data);
	if (!assignable.has_value())
		return {};

	return {DeviceNode{
	    .name = _("Minimum Core Clock"),
	    .interface = *assignable,
	    .hash = md5(data.identifier + "Minimum Core Clock"),
	}};
}

std::vector<TreeNode<DeviceNode>> getSlowdownTemperature(AMDGPUData data) {
	auto string = fileContents(data.hwmonPath + "/temp1_crit");
	if (!string.has_value())
		return {};

	// millicelsius -> celsius
	StaticReadable sr{static_cast<uint>(std::stoi(*string)) / 1000, _("°C")};

	return {DeviceNode{
	    .name = _("Slowdown Temperature"),
	    .interface = sr,
	    .hash = md5(data.identifier + "Slowdown Temperature"),
	}};
}

std::vector<TreeNode<DeviceNode>> getShutdownTemperature(AMDGPUData data) {
	auto string = fileContents(data.hwmonPath + "/temp1_emergency");
	if (!string.has_value())
		return {};

	// millicelsius -> celsius
	StaticReadable sr{static_cast<uint>(std::stoi(*string)) / 1000, _("°C")};

	return {DeviceNode{
	    .name = _("Shutdown Temperature"),
	    .interface = sr,
	    .hash = md5(data.identifier + "Shutdown Temperature"),
	}};
}

// Mysterious undocumented thing that's in at least RX 6000s, see doc/amd-pptables/rx6000
std::vector<TreeNode<DeviceNode>> getCoreVoltageOffset(AMDGPUData data) {
	if (!data.ppTableType.has_value())
		return {};

	auto t = *data.ppTableType;
	if (t != Navi && t != SMU13 && t != Vega20Other)
		return {};

	// TODO: doesn't seem to have a range anywhere
	Range<int> range{-200, 200};
	auto path = data.devPath + "/pp_od_clk_voltage";

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		auto contents = fileContents(path);
		if (!contents.has_value())
			return std::nullopt;

		auto isNewline = [](char c) { return c == '\n'; };
		auto lines = fplus::split_by(isNewline, false, *contents);

		for (int i = 0; i < lines.size(); i++) {
			if (lines[i].find("OD_VDDGFX_OFFSET") != std::string::npos &&
			    i + 1 < lines.size()) {
				// Next line is the value
				auto valueLine = lines[i + 1];
				return std::stoi(valueLine);
			}
		}
		return std::nullopt;
	};

	if (!getFunc().has_value())
		return {};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		std::ofstream file{path};
		char cmdString[32];
		snprintf(cmdString, 32, "vo %i", target);
		if (file.good() && file << cmdString && file << "c")
			return std::nullopt;
		return AssignmentError::UnknownError;
	};

	auto setWithPerfLevel = [=](AssignmentArgument a) {
		return withManualPerformanceLevel(setFunc, a, data);
	};

	Assignable a{setWithPerfLevel, range, getFunc, _("mV")};

	return {DeviceNode{
	    .name = _("Core Voltage Offset"),
	    .interface = a,
	    .hash = md5(data.identifier + "Core Voltage Offset"),
	}};
}

std::vector<TreeNode<DeviceNode>> getUsedVram(AMDGPUData data) {
	auto func = [=]() -> ReadResult {
		uint usedBytes;
		if (amdgpu_query_info(
			data.devHandle, AMDGPU_INFO_VRAM_USAGE, sizeof(usedBytes), &usedBytes) != 0)
			return ReadError::UnknownError;
		// B -> MB
		return usedBytes / 1000000;
	};

	DynamicReadable dr{func, _("MB")};

	if (hasReadableValue(func())) {
		return {DeviceNode{
		    .name = _("Used Memory"),
		    .interface = dr,
		    .hash = md5(data.identifier + "Used VRAM"),
		}};
	}
	return {};
}

std::vector<TreeNode<DeviceNode>> getTotalVram(AMDGPUData data) {
	drm_amdgpu_info_vram_gtt vramInfo;
	if (amdgpu_query_info(data.devHandle, AMDGPU_INFO_VRAM_GTT, sizeof(vramInfo), &vramInfo) !=
	    0)
		return {};

	// B -> MB
	uint totalMBs = vramInfo.vram_size / 1000000;
	StaticReadable sr{totalMBs, _("MB")};

	return {DeviceNode{
	    .name = ("Total Memory"),
	    .interface = sr,
	    .hash = md5(data.identifier + "Total VRAM"),
	}};
}

std::vector<TreeNode<DeviceNode>> getVramRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Video Memory"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "VRAM Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getTemperatureRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Temperatures"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Temperatures"),
	}};
}

std::vector<TreeNode<DeviceNode>> getVoltFreqRoot(AMDGPUData data) {
	if (data.ppTableType.has_value() &&
	    (*data.ppTableType == Navi || *data.ppTableType == SMU13))
		return {DeviceNode{
		    .name = _("Voltage-Frequency Curve"),
		    .interface = std::nullopt,
		    .hash = md5(data.identifier + "Voltage-Frequency Curve"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getClocksRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Clocks"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Clocks"),
	}};
}

std::vector<TreeNode<DeviceNode>> getPerformanceRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Performance"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Performance"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanRoot(AMDGPUData data) {
	return {DeviceNode{
	    .name = _("Fans"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Fans"),
	}};
}

std::vector<TreeNode<DeviceNode>> getPowerRoot(AMDGPUData data) {
	// Root for power usage and power limit
	return {DeviceNode{
	    .name = _("Power"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Power"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCorePStateRoot(AMDGPUData data) {
	if (!data.ppTableType.has_value() || *data.ppTableType != Vega10)
		return {};

	return {DeviceNode{
	    .name = _("Core Performance States"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Core Performance States"),
	}};
}

std::vector<TreeNode<DeviceNode>> getMemoryPStateRoot(AMDGPUData data) {
	if (!data.ppTableType.has_value() || *data.ppTableType != Vega10)
		return {};

	return {DeviceNode{
	    .name = _("Memory Performance States"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Memory Performance States"),
	}};
}

std::vector<TreeNode<DeviceNode>> getUtilizationsRoot(AMDGPUData data) {
	// TODO: PCIe bandwidth utilization missing
	return {DeviceNode{
	    .name = _("Utilizations"),
	    .interface = std::nullopt,
	    .hash = md5(data.identifier + "Utilizations"),
	}};
}

std::vector<TreeNode<DeviceNode>> getGPUName(AMDGPUData data) {
#ifdef WITH_HWDATA
	// Get GPU name from hwdata
	static auto pciObj = getPciObject();
	auto pciData = fromUeventFile(data.deviceFilename);
	if (pciObj.has_value() && pciData.has_value()) {
		auto name = hwdataName(*pciObj, *pciData);
		if (name.has_value())
			return {DeviceNode{
			    .name = *name,
			    .interface = std::nullopt,
			    .hash = md5(data.identifier),
			}};
	}
#endif
	auto name = amdgpu_get_marketing_name(data.devHandle);
	if (name) {
		return {DeviceNode{
		    .name = name,
		    .interface = std::nullopt,
		    .hash = md5(data.identifier),
		}};
	}
	return {};
}

// clang-format off
auto gpuTree = TreeConstructor<AMDGPUData, DeviceNode>{
	getGPUName, {
		{getTemperatureRoot, {
			{getTemperature, {}},
			{getSlowdownTemperature, {}},
			{getShutdownTemperature, {}}
		}},
		{getFanRoot, {
			{getFanMode, {}},
			{getFanSpeedWrite, {}},
			{getFanSpeedWriteRX7000, {}},
			{getFanSpeedRead, {}}
		}},
		{getPowerRoot, {
			{getPowerLimit, {}},
			{getPowerUsage, {}}
		}},
		{getUtilizationsRoot, {
			{getMemoryUtilization, {}},
			{getCoreUtilization, {}}
		}},
		{getVramRoot, {
			{getUsedVram, {}},
			{getTotalVram, {}}
		}},
		{getPerformanceRoot, {
			{getClocksRoot, {
				{getMemoryClockRead, {}},
				{getCoreClockRead, {}},
				{getMinMemoryClock, {}},
				{getMaxMemoryClock, {}},
				{getMinCoreClock, {}},
				{getMaxCoreClock, {}}
			}},
			{getVoltageRead, {}},
			{getCoreVoltageOffset, {}},
			{getForcePerfLevel, {}},
			{getVoltFreqRoot, {
				{getVoltFreqNodes, {
					{getVoltFreqFreq, {}},
					{getVoltFreqVolt, {}}
				}}
			}},
			{getCorePStateRoot, {
				{getCorePStateNodes, {
					{getCorePStateFreq, {}},
					{getCorePStateVolt, {}}
				}}
			}},
			{getMemoryPStateRoot, {
				{getMemoryPStateNodes, {
					{getMemoryPStateFreq, {}},
					{getMemoryPStateVolt, {}}
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
#ifdef WITH_HWDATA
	PythonInstance p;
#endif

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
