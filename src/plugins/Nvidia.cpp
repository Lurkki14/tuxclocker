#include <Crypto.hpp>
#include <Device.hpp>
#include <NVCtrl/NVCtrl.h>
#include <Plugin.hpp>

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

uint nvctrlPerfModes(Display *dpy, uint index) {
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

bool hasReadableValue(ReadResult res) {
	return std::holds_alternative<ReadableValue>(res);
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

	auto maxPerfState = data.maxPerfState.value();
	// TODO: getting current offset and available range doesn't work properly through NVML
	// but setting does
	NVCTRLAttributeValidValuesRec values;
	if (!XNVCTRLQueryValidTargetAttributeValues(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index,
			maxPerfState, NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET, &values))
		return {};

	// Transfer rate -> clock speed
	Range<int> range{values.u.range.min / 2, values.u.range.max / 2};

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
		    data.index, maxPerfState, NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET, &value))
			return std::nullopt;
		return value / 2;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		// Don't need to convert since the function deals with clocks already
		auto ret = nvmlDeviceSetMemClkVfOffset(data.devHandle, target);
		return fromNVMLRet(ret);
	};

	Assignable a{
		setFunc,
		range,
		getFunc,
		"MHz"
	};

	if (getFunc().has_value())
		return {DeviceNode{
			.name = "Memory Clock Offset",
			.interface = a,
			.hash = md5(data.uuid + "Memory Clock Offset"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getCoreClockWrite(NvidiaGPUData data) {
	if (!data.maxPerfState.has_value())
		return {};

	auto maxPerfState = data.maxPerfState.value();
	// TODO: NVML has functions for core clock as well but they're borked as of writing
	NVCTRLAttributeValidValuesRec values;
	if (!XNVCTRLQueryValidTargetAttributeValues(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index,
			maxPerfState, NV_CTRL_GPU_NVCLOCK_OFFSET, &values)) {
		std::cout << "b" << maxPerfState << "\n";
		return {};
	}

	// Transfer rate -> clock speed
	Range<int> range{values.u.range.min, values.u.range.max};

	auto getFunc = [=]() -> std::optional<AssignmentArgument> {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
		    data.index, maxPerfState, NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET, &value))
			return std::nullopt;
		return value;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<int>(a))
			return AssignmentError::InvalidType;
		auto target = std::get<int>(a);
		if (target < range.min || target > range.max)
			return AssignmentError::OutOfRange;

		if (!XNVCTRLSetTargetAttributeAndGetStatus(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
				data.index, maxPerfState, NV_CTRL_GPU_NVCLOCK_OFFSET, target))
			return AssignmentError::UnknownError;
		return std::nullopt;
	};

	Assignable a{
		setFunc,
		range,
		getFunc,
		"MHz"
	};

	return {DeviceNode{
		.name = "Core Clock Offset",
		.interface = a,
		.hash = md5(data.uuid + "Core Clock Offset"),
	}};
}

std::vector<TreeNode<DeviceNode>> getCoreClockRead(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult{
		uint clock;
		nvmlReturn_t ret;
		ret = nvmlDeviceGetClockInfo(data.devHandle, NVML_CLOCK_GRAPHICS, &clock);
		if (ret == NVML_SUCCESS)
			return clock;
		return fromNVMLError(ret);
	};

	DynamicReadable dr{func, "MHz"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Core Clock",
			.interface = dr,
			.hash = md5(data.uuid + "Core Clock"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getMemClockRead(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult{
		uint clock;
		nvmlReturn_t ret;
		ret = nvmlDeviceGetClockInfo(data.devHandle, NVML_CLOCK_MEM, &clock);
		if (ret == NVML_SUCCESS)
			return clock;
		return fromNVMLError(ret);
	};

	DynamicReadable dr{func, "MHz"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Memory Clock",
			.interface = dr,
			.hash = md5(data.uuid + "Memory Clock"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getClocksRoot(NvidiaGPUData data) {
	// TODO: this gets added unconditionally
	// If leaf nodes without an interface become a problem, we can remove them
	// centrally in the daemon.
	// Another option would be to check from here if any children would get added,
	// but that's spaghetti and inefficient.
	return {DeviceNode{
		.name = "Clocks",
		.interface = std::nullopt,
		.hash = md5(data.uuid + "Clocks"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedRead(NvidiaGPUData data) {
	static uint fanId = 0;
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

	DynamicReadable dr{func, "%"};

	fanId++;
	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Fan Speed",
			.interface = dr,
			.hash = md5(data.uuid + "Fan Speed Read" + std::to_string(id)),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getFanSpeedWrite(NvidiaGPUData data) {
	static uint fanId = 0;
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

	Assignable a{setFunc, Range<int>{0, 100}, getFunc, "%"};

	fanId++;
	if (getFunc().has_value())
		return {DeviceNode{
			.name = "Fan Speed",
			.interface = a,
			.hash = md5(data.uuid + "Fan Speed Write" + std::to_string(id)),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getFanMode(NvidiaGPUData data) {
	static uint fanId = 0;
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
	EnumerationVec opts = {{"Automatic", 0}, {"Manual", 1}};

	Assignable a{setFunc, opts, getFunc, std::nullopt};

	fanId++;
	return {DeviceNode{
		.name = "Fan Mode",
		.interface = a,
		.hash = md5(data.uuid + "Fan Mode" + std::to_string(id)),
	}};
}

std::vector<TreeNode<DeviceNode>> getCoreUtilization(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult{
		nvmlUtilization_t value;
		auto ret = nvmlDeviceGetUtilizationRates(data.devHandle, &value);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return value.gpu;
	};

	DynamicReadable dr{func, "%"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Core Utilization",
			.interface = dr,
			.hash = md5(data.uuid + "Core Utilization"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getMemoryUtilization(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult{
		nvmlUtilization_t value;
		auto ret = nvmlDeviceGetUtilizationRates(data.devHandle, &value);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return value.memory;
	};

	DynamicReadable dr{func, "%"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Memory Utilization",
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

	auto func = [=]() -> ReadResult{
		uint rx, tx;
		auto rxret = nvmlDeviceGetPcieThroughput(data.devHandle, NVML_PCIE_UTIL_RX_BYTES, &rx);
		auto txret = nvmlDeviceGetPcieThroughput(data.devHandle, NVML_PCIE_UTIL_TX_BYTES, &tx);

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

	DynamicReadable dr{func, "%"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "PCIe Bandwidth Utilization",
			.interface = dr,
			.hash = md5(data.uuid + "PCIe Bandwidth Utilization"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getPowerUsage(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult{
		uint value;
		auto ret = nvmlDeviceGetPowerUsage(data.devHandle, &value);
		if (ret != NVML_SUCCESS)
			return fromNVMLError(ret);
		return static_cast<double>(value) / 1000;
	};

	DynamicReadable dr{func, "W"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Power Usage",
			.interface = dr,
			.hash = md5(data.uuid + "Power Usage"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getPowerLimit(NvidiaGPUData data) {
	uint min, max;
	if (nvmlDeviceGetPowerManagementLimitConstraints(data.devHandle, &min, &max)
			!= NVML_SUCCESS)
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

	Assignable a{setFunc, range, getFunc, "W"};

	return {DeviceNode{
		.name = "Power Limit",
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

	DynamicReadable dr{func, "°C"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Temperature",
			.interface = dr,
			.hash = md5(data.uuid + "Temperature"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getSlowdownTemperature(NvidiaGPUData data) {
	uint temp;
	if (nvmlDeviceGetTemperatureThreshold(data.devHandle,
			NVML_TEMPERATURE_THRESHOLD_SLOWDOWN, &temp) != NVML_SUCCESS)
		return {};

	StaticReadable sr{temp, "°C"};

	return {DeviceNode{
		.name = "Slowdown Temperature",
		.interface = sr,
		.hash = md5(data.uuid + "Slowdown Temperature"),
	}};
}

std::vector<TreeNode<DeviceNode>> getShutdownTemperature(NvidiaGPUData data) {
	uint temp;
	if (nvmlDeviceGetTemperatureThreshold(data.devHandle,
			NVML_TEMPERATURE_THRESHOLD_SHUTDOWN, &temp) != NVML_SUCCESS)
		return {};

	StaticReadable sr{temp, "°C"};

	return {DeviceNode{
		.name = "Shutdown Temperature",
		.interface = sr,
		.hash = md5(data.uuid + "Shutdown Temperature"),
	}};
}

std::vector<TreeNode<DeviceNode>> getVoltage(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU, data.index,
				0, NV_CTRL_GPU_CURRENT_CORE_VOLTAGE, &value))
			return ReadError::UnknownError;
		return static_cast<double>(value) / 1000;
	};

	DynamicReadable dr{func, "mV"};

	if (hasReadableValue(func()))
		return {DeviceNode{
			.name = "Core Voltage",
			.interface = dr,
			.hash = md5(data.uuid + "Core Voltage"),
		}};
	return {};
}

std::vector<TreeNode<DeviceNode>> getVoltageOffset(NvidiaGPUData data) {
	NVCTRLAttributeValidValuesRec values;
	if (!XNVCTRLQueryValidTargetAttributeValues(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
			data.index, 0, NV_CTRL_GPU_OVER_VOLTAGE_OFFSET, &values))
		return {};

	// uV -> mV
	Range<int> range{values.u.range.min / 1000, values.u.range.max / 1000};

	auto getFunc = [data]() -> std::optional<AssignmentArgument> {
		int value;
		if (!XNVCTRLQueryTargetAttribute(data.dpy, NV_CTRL_TARGET_TYPE_GPU,
				data.index, 0, NV_CTRL_GPU_OVER_VOLTAGE_OFFSET, &value))
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

	Assignable a{setFunc, range, getFunc, "mV"};

	return {DeviceNode{
		.name = "Core Voltage Offset",
		.interface = a,
		.hash = md5(data.uuid + "Core Voltage Offset"),
	}};
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
			.hash = md5(data.uuid + "Fan" + index),
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
		.name = "Temperatures",
		.interface = std::nullopt,
		.hash = md5(data.uuid + "Temperatures"),
	}};
}

std::vector<TreeNode<DeviceNode>> getFanRoot(NvidiaGPUData data) {
	return {DeviceNode{
		.name = "Fans",
		.interface = std::nullopt,
		.hash = md5(data.uuid + "Fans"),
	}};
}

std::vector<TreeNode<DeviceNode>> getUtilizationsRoot(NvidiaGPUData data) {
	return {DeviceNode{
		.name = "Utilizations",
		.interface = std::nullopt,
		.hash = md5(data.uuid + "Utilizations"),
	}};
}

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
			{getMemClockWrite, {}}
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
		{getPowerUsage, {}},
		{getPowerLimit, {}},
		{getVoltage, {}},
		{getVoltageOffset, {}}
	}
};

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

NvidiaPlugin::NvidiaPlugin() : m_dpy() {
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
