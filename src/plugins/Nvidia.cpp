#include <Crypto.hpp>
#include <Device.hpp>
#include <libintl.h>
#include <NVCtrl/NVCtrl.h>
#include <Plugin.hpp>
#include <Utils.hpp>

#include <fplus/fplus.hpp>
#include <iostream>
#include <optional>
#include <patterns.hpp>
#include <TreeConstructor.hpp>
#include <type_traits>
// Need to break alphabetic order since nvidia couldn't put the required includes in their header
#include <X11/Xlib.h> // Also breaks everything if it's in a different spot
#include <NVCtrl/NVCtrlLib.h>
#include <nvml.h>

#define _(String) gettext(String)

using namespace TuxClocker;
using namespace TuxClocker::Crypto;
using namespace TuxClocker::Device;
using namespace TuxClocker::Plugin;
using namespace mpark::patterns;

// This data is used to construct the tree
struct NvidiaGPUData {
	nvmlDevice_t devHandle;
	Display *dpy;
	uint index;
	std::string uuid;
	std::optional<uint> maxPerfState;
	uint fanCount;
};

ReadError fromNVMLError(nvmlReturn_t err) {
	// TODO: add conversions
	switch (err) {
	default:
		return ReadError::UnknownError;
	}
	return ReadError::UnknownError;
}

std::optional<AssignmentError> fromNVMLRet(nvmlReturn_t ret) {
	if (ret == NVML_SUCCESS)
		return std::nullopt;
	// TODO: more conversions
	return AssignmentError::UnknownError;
}

std::optional<uint> nvctrlPerfModes(Display *dpy, uint index) {
	if (!dpy)
		return std::nullopt;

	// TODO: NVML has a function to get these but is borked
	// Thanks to Artifth for original code
	char *result;
	if (XNVCTRLQueryTargetStringAttribute(dpy, NV_CTRL_TARGET_TYPE_GPU, index, 0,
		NV_CTRL_STRING_PERFORMANCE_MODES, &result)) {
		auto s = std::string(result);
		auto modes = std::count(s.begin(), s.end(), ';');
		delete result;
		return modes;
	}
	// Usually there's 3 perf modes
	return 3;
}

uint nvmlFanCount(nvmlDevice_t dev) {
	uint fanCount;
	if (nvmlDeviceGetNumFans(dev, &fanCount) != NVML_SUCCESS)
		return 0;
	return fanCount;
}

std::optional<NvidiaGPUData> fromIndex(Display *dpy, uint i) {
	nvmlDevice_t dev;
	if (nvmlDeviceGetHandleByIndex_v2(i, &dev) != NVML_SUCCESS) {
		std::cout << "nvidia: couldn't get nvml handle for index " << i << "\n";
		return std::nullopt;
	}
	char uuid[NVML_DEVICE_UUID_BUFFER_SIZE];
	if (nvmlDeviceGetUUID(dev, uuid, NVML_DEVICE_UUID_BUFFER_SIZE) != NVML_SUCCESS)
		return std::nullopt;

	return NvidiaGPUData{
	    .devHandle = dev,
	    .index = i,
	    .uuid = uuid,
	    .maxPerfState = nvctrlPerfModes(dpy, i),
	    .fanCount = nvmlFanCount(dev),
	};
}

bool isXorg() {
	auto value = std::getenv("XDG_SESSION_TYPE");
	if (!value)
		// Assume X, since that will lead to the least amount of broken
		return true;

	return std::string{value} == "x11";
}

std::vector<TreeNode<DeviceNode>> getGPUName(NvidiaGPUData data) {
	char name[NVML_DEVICE_NAME_BUFFER_SIZE];
	if (nvmlDeviceGetName(data.devHandle, name, NVML_DEVICE_NAME_BUFFER_SIZE) == NVML_SUCCESS)
		return {DeviceNode{
		    .name = name,
		    .interface = std::nullopt,
		    .hash = md5(data.uuid),
		}};
	// Use GPU index for name, eg. "GPU 0"
	// Hopefully no one has over 999 GPUs :D
	char buf[8];
	snprintf(buf, 8, "GPU %u", data.index);
	return {DeviceNode{
	    .name = buf,
	    .interface = std::nullopt,
	    .hash = md5(data.uuid),
	}};
}

std::vector<TreeNode<DeviceNode>> getMemClockWrite(NvidiaGPUData data) {
	if (!data.maxPerfState.has_value())
		return {};

	// TODO: getting current offset and available range doesn't work properly through NVML
	// but setting does
	auto maxPerfState = data.maxPerfState.value();
	auto getFunc = [=](int attribute) -> std::optional<AssignmentArgument> {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index,
			maxPerfState, attribute, &value))
			return std::nullopt;
		return value / 2;
	};

	// Check which attribute works (same as nvidia-settings code)
	std::optional<int> attribute;
	if (getFunc(NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET).has_value())
		attribute = NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET;
	if (getFunc(NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET_ALL_PERFORMANCE_LEVELS).has_value())
		attribute = NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET_ALL_PERFORMANCE_LEVELS;

	if (!attribute.has_value())
		return {};

	NVCTRLAttributeValidValuesRec values;
	if (!XNVCTRLQueryValidTargetAttributeValues(
		data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index, maxPerfState, *attribute, &values)) {
		return {};
	}
	// Transfer rate -> clock speed
	Range<int> range{values.u.range.min / 2, values.u.range.max / 2};

	// Seems the ..ALL_PERFORMANCE_LEVELS attribute doesn't work with the NVML function
	auto setFuncNVML = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		// Don't need to convert since the function deals with clocks already
		auto ret = nvmlDeviceSetMemClkVfOffset(data.devHandle, target);
		return fromNVMLRet(ret);
	};

	auto setFuncXNVCTRL = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		// Clock speed -> transfer rate
		auto value = target * 2;
		if (!XNVCTRLSetTargetAttributeAndGetStatus(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
			data.index, maxPerfState, *attribute, value))
			return AssignmentError::UnknownError;
		return std::nullopt;
	};

	auto getFuncWithAttribute = [=]() -> std::optional<AssignmentArgument> {
		return getFunc(*attribute);
	};

	auto a = (*attribute == NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET)
		     ? Assignable{setFuncNVML, range, getFuncWithAttribute, _("MHz")}
		     : Assignable{setFuncXNVCTRL, range, getFuncWithAttribute, _("MHz")};

	return {DeviceNode{
	    .name = _("Memory Clock Offset"),
	    .interface = a,
	    .hash = md5(data.uuid + "Memory Clock Offset"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCoreClockWrite(NvidiaGPUData data) {
	if (!data.maxPerfState.has_value())
		return {};

	auto maxPerfState = data.maxPerfState.value();
	auto getFunc = [=](int attribute) -> std::optional<AssignmentArgument> {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index,
			maxPerfState, attribute, &value))
			return std::nullopt;
		return value;
	};

	// Check which attribute works (same as nvidia-settings code)
	std::optional<int> attribute;
	if (getFunc(NV_CTRL_GPU_NVCLOCK_OFFSET).has_value())
		attribute = NV_CTRL_GPU_NVCLOCK_OFFSET;
	if (getFunc(NV_CTRL_GPU_NVCLOCK_OFFSET_ALL_PERFORMANCE_LEVELS).has_value())
		attribute = NV_CTRL_GPU_NVCLOCK_OFFSET_ALL_PERFORMANCE_LEVELS;

	if (!attribute.has_value())
		return {};

	// TODO: NVML has functions for core clock as well but they're borked as of writing
	NVCTRLAttributeValidValuesRec values;
	if (!XNVCTRLQueryValidTargetAttributeValues(
		data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index, maxPerfState, *attribute, &values)) {
		return {};
	}

	Range<int> range{values.u.range.min, values.u.range.max};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		if (!XNVCTRLSetTargetAttributeAndGetStatus(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
			data.index, maxPerfState, *attribute, target))
			return AssignmentError::UnknownError;
		return std::nullopt;
	};

	auto getFuncWithAttribute = [=]() -> std::optional<AssignmentArgument> {
		return getFunc(*attribute);
	};

	Assignable a{setFunc, range, getFuncWithAttribute, _("MHz")};

	return {DeviceNode{
	    .name = _("Core Clock Offset"),
	    .interface = a,
	    .hash = md5(data.uuid + "Core Clock Offset"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCoreClockWriteNoX(NvidiaGPUData data) {
	// Try to use NVML function only on non-X, since seems to be broken on some GPUs
	if (isXorg())
		return {};

	// Completely made-up range
	Range<int> range{-300, 1000};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		auto retval = nvmlDeviceSetGpcClkVfOffset(data.devHandle, target);
		return fromNVMLRet(retval);
	};

	// No way to get through NVML by the looks if it
	auto getFunc = []() { return 0; };

	return {DeviceNode{
	    .name = _("Core Clock Offset"),
	    .interface = Assignable{setFunc, range, getFunc, _("MHz")},
	    .hash = md5(data.uuid + "Core Clock Offset NVML"),
	}};
}

std::vector<TreeNode<DeviceNode>> getMemoryClockWriteNoX(NvidiaGPUData data) {
	// Try to use NVML function only on non-X, since seems to be broken on some GPUs
	if (isXorg())
		return {};

	// Completely made-up range
	Range<int> range{-800, 1000};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		auto retval = nvmlDeviceSetMemClkVfOffset(data.devHandle, target);
		return fromNVMLRet(retval);
	};

	// No way to get through NVML by the looks if it
	auto getFunc = []() { return 0; };

	return {DeviceNode{
	    .name = _("Memory Clock Offset"),
	    .interface = Assignable{setFunc, range, getFunc, _("MHz")},
	    .hash = md5(data.uuid + "Memory Clock Offset NVML"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCoreClockRead(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		uint clock;
		nvmlReturn_t ret;
		ret = nvmlDeviceGetClockInfo(data.devHandle, NVML_CLOCK_GRAPHICS, &clock);
		if (ret == NVML_SUCCESS)
			return clock;
		return fromNVMLError(ret);
	};

	DynamicReadable dr{func, _("MHz")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Core Clock"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Core Clock"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getMemClockRead(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		uint clock;
		nvmlReturn_t ret;
		ret = nvmlDeviceGetClockInfo(data.devHandle, NVML_CLOCK_MEM, &clock);
		if (ret == NVML_SUCCESS)
			return clock;
		return fromNVMLError(ret);
	};

	DynamicReadable dr{func, _("MHz")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Memory Clock"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Memory Clock"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getTotalVram(NvidiaGPUData data) {
	nvmlMemory_t mem;
	if (nvmlDeviceGetMemoryInfo(data.devHandle, &mem) != NVML_SUCCESS)
		return {};
	// B -> MB
	auto total = static_cast<uint>(mem.total / 1000000ull);

	StaticReadable sr{total, _("MB")};

	return {DeviceNode{
	    .name = _("Total Memory"),
	    .interface = sr,
	    .hash = md5(data.uuid + "Total VRAM"),
	}};
}

std::vector<TreeNode<DeviceNode>> getReservedVram(NvidiaGPUData data) {
	auto func = [=]() -> ReadResult {
		nvmlMemory_v2_t mem;
		if (nvmlDeviceGetMemoryInfo_v2(data.devHandle, &mem) != NVML_SUCCESS)
			return ReadError::UnknownError;
		// B -> MB
		return static_cast<uint>(mem.reserved / 1000000ull);
	};

	DynamicReadable dr{func, _("MB")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Reserved Memory"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Reserved VRAM"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getUsedVram(NvidiaGPUData data) {
	auto func = [=]() -> ReadResult {
		nvmlMemory_t mem;
		if (nvmlDeviceGetMemoryInfo(data.devHandle, &mem) != NVML_SUCCESS)
			return ReadError::UnknownError;
		// B -> MB
		return static_cast<uint>(mem.used / 1000000ull);
	};

	DynamicReadable dr{func, _("MB")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Used Memory"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Used VRAM"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getVramRoot(NvidiaGPUData data) {
	return {DeviceNode{
	    .name = _("Video Memory"),
	    .interface = std::nullopt,
	    .hash = md5(data.uuid + "VRAM Root"),
	}};
}

std::vector<TreeNode<DeviceNode>> getClocksRoot(NvidiaGPUData data) {
	// TODO: this gets added unconditionally
	// If leaf nodes without an interface become a problem, we can remove them
	// centrally in the daemon.
	// Another option would be to check from here if any children would get added,
	// but that's spaghetti and inefficient.
	return {DeviceNode{
	    .name = _("Clocks"),
	    .interface = std::nullopt,
	    .hash = md5(data.uuid + "Clocks"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedRead(NvidiaGPUData data) {
	static nvmlDevice_t latestDev = nullptr;
	static uint fanId = 0;

	if (data.devHandle != latestDev)
		// Start from zero for newly seen device
		fanId = 0;

	latestDev = data.devHandle;

	if (data.fanCount == 0 || fanId + 1 > data.fanCount)
		return {};

	auto id = fanId;
	auto func = [=]() -> ReadResult {
		uint speed;
		auto ret = nvmlDeviceGetFanSpeed_v2(data.devHandle, id, &speed);
		if (ret == NVML_SUCCESS)
			return speed;
		return fromNVMLError(ret);
	};

	DynamicReadable dr{func, _("%")};

	fanId++;
	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Fan Speed"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Fan Speed Read" + std::to_string(id)),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedWrite(NvidiaGPUData data) {
	static nvmlDevice_t latestDev = nullptr;
	static uint fanId = 0;

	if (data.devHandle != latestDev)
		// Start from zero for newly seen device
		fanId = 0;

	latestDev = data.devHandle;

	if (data.fanCount == 0 || fanId + 1 > data.fanCount)
		return {};

	auto id = fanId;
	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		uint target;
		if (nvmlDeviceGetTargetFanSpeed(data.devHandle, id, &target) == NVML_SUCCESS)
			return target;
		return std::nullopt;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < 0 || target > 100)
			return AssignmentError::OutOfRange;
		auto ret = nvmlDeviceSetFanSpeed_v2(data.devHandle, id, target);
		return fromNVMLRet(ret);
	};

	Assignable a{setFunc, Range<int>{0, 100}, getFunc, _("%")};

	fanId++;
	if (getFunc().has_value())
		return {DeviceNode{
		    .name = _("Fan Speed"),
		    .interface = a,
		    .hash = md5(data.uuid + "Fan Speed Write" + std::to_string(id)),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getFanMode(NvidiaGPUData data) {
	static nvmlDevice_t latestDev = nullptr;
	static uint fanId = 0;

	if (data.devHandle != latestDev)
		// Start from zero for newly seen device
		fanId = 0;

	latestDev = data.devHandle;

	if (data.fanCount == 0 || fanId + 1 > data.fanCount)
		return {};

	auto id = fanId;
	// TODO: actually check the fan control mode
	// NVML supposedly has a function for this, (nvmlDeviceGetFanControlPolicy_v2),
	// but doesn't seem to exist in v.412
	auto getFunc = []() -> std::optional<AssignmentArgument> { return 0u; };

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<uint>(a))
			return AssignmentError::InvalidType;
		uint target = std::get<uint>(a);
		if (target > 1)
			return AssignmentError::OutOfRange;

		if (target == 0) {
			auto ret = nvmlDeviceSetDefaultFanSpeed_v2(data.devHandle, id);
			return fromNVMLRet(ret);
		}
		// For now 'Manual' means getting current target and setting manually to that
		uint current;
		if (nvmlDeviceGetTargetFanSpeed(data.devHandle, id, &current) != NVML_SUCCESS)
			return AssignmentError::UnknownError;
		auto ret = nvmlDeviceSetFanSpeed_v2(data.devHandle, id, current);
		return fromNVMLRet(ret);
	};
	EnumerationVec opts = {{_("Automatic"), 0}, {_("Manual"), 1}};

	Assignable a{setFunc, opts, getFunc, std::nullopt};

	fanId++;
	return {DeviceNode{
	    .name = _("Fan Mode"),
	    .interface = a,
	    .hash = md5(data.uuid + "Fan Mode" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getCoreUtilization(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		nvmlUtilization_t value;
		auto ret = nvmlDeviceGetUtilizationRates(data.devHandle, &value);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return value.gpu;
	};

	DynamicReadable dr{func, _("%")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Core Utilization"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Core Utilization"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getMemoryUtilization(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		nvmlUtilization_t value;
		auto ret = nvmlDeviceGetUtilizationRates(data.devHandle, &value);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return value.memory;
	};

	DynamicReadable dr{func, _("%")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Memory Utilization"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Memory Utilization"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getPcieUtilization(NvidiaGPUData data) {
	uint linkSpeed, linkWidth;
	auto widthRet = nvmlDeviceGetCurrPcieLinkWidth(data.devHandle, &linkWidth);
	auto speedRet = nvmlDeviceGetPcieSpeed(data.devHandle, &linkSpeed);
	if (speedRet != NVML_SUCCESS || widthRet != NVML_SUCCESS)
		return {};

	auto func = [=]() -> ReadResult {
		uint rx, tx;
		auto rxret =
		    nvmlDeviceGetPcieThroughput(data.devHandle, NVML_PCIE_UTIL_RX_BYTES, &rx);
		auto txret =
		    nvmlDeviceGetPcieThroughput(data.devHandle, NVML_PCIE_UTIL_TX_BYTES, &tx);

		if (txret == NVML_SUCCESS && rxret == NVML_SUCCESS) {
			auto totalMBs = (rx + tx) / 1000;
			auto maxMBs = (linkSpeed / 8) * linkWidth;
			auto percentage =
			    (static_cast<double>(totalMBs) / static_cast<double>(maxMBs)) * 100;
			// We could display this more granularly now, but is that really needed?
			return static_cast<uint>(round(percentage));
		}
		return ReadError::UnknownError;
	};

	DynamicReadable dr{func, _("%")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("PCIe Bandwidth Utilization"),
		    .interface = dr,
		    .hash = md5(data.uuid + "PCIe Bandwidth Utilization"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getPowerUsage(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		uint value;
		auto ret = nvmlDeviceGetPowerUsage(data.devHandle, &value);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return static_cast<double>(value) / 1000;
	};

	DynamicReadable dr{func, _("W")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Power Usage"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Power Usage"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getPowerLimit(NvidiaGPUData data) {
	uint min, max;
	if (nvmlDeviceGetPowerManagementLimitConstraints(data.devHandle, &min, &max) !=
	    NVML_SUCCESS)
		return {};

	// mW -> W
	Range<double> range{static_cast<double>(min) / 1000, static_cast<double>(max) / 1000};

	auto getFunc = [data]() -> std::optional<AssignmentArgument> {
		uint limit;
		auto ret = nvmlDeviceGetPowerManagementLimit(data.devHandle, &limit);
		if (ret != NVML_SUCCESS)
			return std::nullopt;
		return static_cast<double>(limit) / 1000;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<double>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<double>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		// W -> mW
		auto ret = nvmlDeviceSetPowerManagementLimit(data.devHandle, round(target * 1000));
		return fromNVMLRet(ret);
	};

	Assignable a{setFunc, range, getFunc, _("W")};

	return {DeviceNode{
	    .name = _("Power Limit"),
	    .interface = a,
	    .hash = md5(data.uuid + "Power Limit"),
	}};
}

std::vector<TreeNode<DeviceNode>> getTemperature(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		uint temp;
		auto ret = nvmlDeviceGetTemperature(data.devHandle, NVML_TEMPERATURE_GPU, &temp);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return temp;
	};

	DynamicReadable dr{func, _("°C")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Temperature"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Temperature"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getSlowdownTemperature(NvidiaGPUData data) {
	uint temp;
	if (nvmlDeviceGetTemperatureThreshold(
		data.devHandle, NVML_TEMPERATURE_THRESHOLD_SLOWDOWN, &temp) != NVML_SUCCESS)
		return {};

	StaticReadable sr{temp, _("°C")};

	return {DeviceNode{
	    .name = _("Slowdown Temperature"),
	    .interface = sr,
	    .hash = md5(data.uuid + "Slowdown Temperature"),
	}};
}

std::vector<TreeNode<DeviceNode>> getShutdownTemperature(NvidiaGPUData data) {
	uint temp;
	if (nvmlDeviceGetTemperatureThreshold(
		data.devHandle, NVML_TEMPERATURE_THRESHOLD_SHUTDOWN, &temp) != NVML_SUCCESS)
		return {};

	StaticReadable sr{temp, _("°C")};

	return {DeviceNode{
	    .name = _("Shutdown Temperature"),
	    .interface = sr,
	    .hash = md5(data.uuid + "Shutdown Temperature"),
	}};
}

std::vector<TreeNode<DeviceNode>> getVoltage(NvidiaGPUData data) {
	if (!data.dpy)
		return {};

	auto func = [data]() -> ReadResult {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index, 0,
			NV_CTRL_GPU_CURRENT_CORE_VOLTAGE, &value))
			return ReadError::UnknownError;
		return static_cast<double>(value) / 1000;
	};

	DynamicReadable dr{func, _("mV")};

	if (hasReadableValue(func()))
		return {DeviceNode{
		    .name = _("Core Voltage"),
		    .interface = dr,
		    .hash = md5(data.uuid + "Core Voltage"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getVoltageOffset(NvidiaGPUData data) {
	if (!data.dpy)
		return {};

	NVCTRLAttributeValidValuesRec values;
	if (!XNVCTRLQueryValidTargetAttributeValues(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index,
		0, NV_CTRL_GPU_OVER_VOLTAGE_OFFSET, &values))
		return {};

	// uV -> mV
	Range<int> range{values.u.range.min / 1000, values.u.range.max / 1000};

	auto getFunc = [data]() -> std::optional<AssignmentArgument> {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index, 0,
			NV_CTRL_GPU_OVER_VOLTAGE_OFFSET, &value))
			return std::nullopt;
		// uV -> mV
		return value / 1000;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;

		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		// mV -> uV
		if (!XNVCTRLSetTargetAttributeAndGetStatus(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
			data.index, 0, NV_CTRL_GPU_OVER_VOLTAGE_OFFSET, target * 1000))
			return AssignmentError::UnknownError;
		return std::nullopt;
	};

	Assignable a{setFunc, range, getFunc, _("mV")};

	if (getFunc().has_value())
		return {DeviceNode{
		    .name = _("Core Voltage Offset"),
		    .interface = a,
		    .hash = md5(data.uuid + "Core Voltage Offset"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getMultiFanRoots(NvidiaGPUData data) {
	if (data.fanCount < 2)
		return {};

	std::vector<TreeNode<DeviceNode>> nodes;
	for (uint i = 0; i < data.fanCount; i++) {
		auto index = std::to_string(i);
		nodes.push_back(DeviceNode{
		    .name = index,
		    .interface = std::nullopt,
		    .hash = md5(data.uuid + _("Fan") + index),
		});
	}
	return nodes;
}
std::vector<TreeNode<DeviceNode>> getSingleFanSpeedWrite(NvidiaGPUData data) {
	if (data.fanCount != 1)
		return {};

	return getFanSpeedWrite(data);
}

std::vector<TreeNode<DeviceNode>> getSingleFanSpeedRead(NvidiaGPUData data) {
	// If we reach this with multiple fans, the static fan index will be too big anyway,
	// but better to be explicit
	if (data.fanCount != 1)
		return {};

	return getFanSpeedRead(data);
}

std::vector<TreeNode<DeviceNode>> getSingleFanMode(NvidiaGPUData data) {
	if (data.fanCount != 1)
		return {};

	return getFanMode(data);
}

std::vector<TreeNode<DeviceNode>> getTemperaturesRoot(NvidiaGPUData data) {
	return {DeviceNode{
	    .name = _("Temperatures"),
	    .interface = std::nullopt,
	    .hash = md5(data.uuid + "Temperatures"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanRoot(NvidiaGPUData data) {
	return {DeviceNode{
	    .name = _("Fans"),
	    .interface = std::nullopt,
	    .hash = md5(data.uuid + "Fans"),
	}};
}

std::vector<TreeNode<DeviceNode>> getUtilizationsRoot(NvidiaGPUData data) {
	return {DeviceNode{
	    .name = _("Utilizations"),
	    .interface = std::nullopt,
	    .hash = md5(data.uuid + "Utilizations"),
	}};
}

// clang-format wants to cram some this to the same line
// clang-format off
auto gpuTree = TreeConstructor<NvidiaGPUData, DeviceNode>{
	getGPUName, {
		{getUtilizationsRoot, {
			{getPcieUtilization, {}},
			{getCoreUtilization, {}},
			{getMemoryUtilization, {}},
		}},
		{getTemperaturesRoot, {
			{getTemperature, {}},
			{getSlowdownTemperature, {}},
			{getShutdownTemperature, {}}
		}},
		{getClocksRoot, {
			{getCoreClockRead, {}},
			{getCoreClockWrite, {}},
			{getMemClockRead, {}},
			{getMemClockWrite, {}},
			{getCoreClockWriteNoX, {}},
			{getMemoryClockWriteNoX, {}}
		}},
		{getFanRoot, {
			{getMultiFanRoots, {
				{getFanSpeedWrite, {}},
				{getFanSpeedRead, {}},
				{getFanMode, {}}
			}},
			{getSingleFanSpeedRead, {}},
			{getSingleFanSpeedWrite, {}},
			{getSingleFanMode, {}}
		}},
		{getVramRoot, {
			{getTotalVram, {}},
			{getUsedVram, {}},
			{getReservedVram, {}}
		}},
		{getPowerUsage, {}},
		{getPowerLimit, {}},
		{getVoltage, {}},
		{getVoltageOffset, {}}
	}
};
// clang-format on

class NvidiaPlugin : public DevicePlugin {
public:
	NvidiaPlugin();
	~NvidiaPlugin();
	std::optional<InitializationError> initializationError() { return std::nullopt; }
	TreeNode<DeviceNode> deviceRootNode();
private:
	Display *m_dpy;
};

NvidiaPlugin::~NvidiaPlugin() {
	nvmlShutdown();
	if (m_dpy)
		XCloseDisplay(m_dpy);
}

NvidiaPlugin::NvidiaPlugin() {
	m_dpy = nullptr;
	// NOTE: we don't seem to need to do any locale stuff here,
	// since the daemon does and loads us
	if (nvmlInit_v2() != NVML_SUCCESS)
		std::cout << "nvidia: couldn't initialize NVML!\n";

	m_dpy = XOpenDisplay(NULL);
	if (!m_dpy)
		std::cout << "nvidia: Couldn't open X display!\n";
}

TreeNode<DeviceNode> NvidiaPlugin::deviceRootNode() {
	// Root node is invisible
	TreeNode<DeviceNode> root;

	uint gpuCount;
	if (nvmlDeviceGetCount(&gpuCount) != NVML_SUCCESS) {
		std::cout << "nvidia: couldn't get GPU count from NVML!\n";
		return root;
	}
	std::vector<NvidiaGPUData> gpuData;
	for (uint i = 0; i < gpuCount; i++) {
		auto data = fromIndex(m_dpy, i);
		if (data.has_value()) {
			data->dpy = m_dpy;
			gpuData.push_back(data.value());
		}
	}

	for (auto &datum : gpuData)
		constructTree(gpuTree, root, datum);

	return root;
}

TUXCLOCKER_PLUGIN_EXPORT(NvidiaPlugin)
