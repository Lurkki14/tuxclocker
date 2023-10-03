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

std::optional<uint> nvmlMaxPerfState(nvmlDevice_t dev) {
	nvmlPstates_t pstates[NVML_MAX_GPU_PERF_PSTATES];
	if (nvmlDeviceGetSupportedPerformanceStates(dev, pstates, NVML_MAX_GPU_PERF_PSTATES)
			!= NVML_SUCCESS)
		return std::nullopt;

	uint max = 0;
	for (int i = 0; i < NVML_MAX_GPU_PERF_PSTATES; i++) {
		int state = pstates[i];
		if (state > max && state != NVML_PSTATE_UNKNOWN)
			max = state;
	}
	return max;
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

std::optional<NvidiaGPUData> fromIndex(uint i) {
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
		.maxPerfState = nvmlMaxPerfState(dev),
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






	};


std::vector<TreeNode<DeviceNode>> getCoreClockRead(NvidiaGPUData data) {
	auto func = [data]() -> ReadResult{
		uint clock;
		nvmlReturn_t ret;
		ret = nvmlDeviceGetClockInfo(data.devHandle, NVML_CLOCK_GRAPHICS, &clock);
		if (ret == NVML_SUCCESS)
			return clock;
		return fromNVMLError(ret);
	};


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


std::vector<TreeNode<DeviceNode>> getFanRoot(NvidiaGPUData data) {
	return {DeviceNode{
		.name = "Fans",
		.interface = std::nullopt,
		.hash = md5(data.uuid + "Fans"),
	}};
}

auto gpuTree = TreeConstructor<NvidiaGPUData, DeviceNode>{
	getGPUName, {
		{getFanRoot, {
			{getMultiFanRoots, {
				{getFanSpeedWrite, {}},
				{getFanSpeedRead, {}},
				{getFanMode, {}}
			}},
			{getSingleFanSpeedRead, {}},
			{getSingleFanSpeedWrite, {}},
			{getSingleFanMode, {}}
		}}
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
		auto data = fromIndex(i);
		if (data.has_value())
			gpuData.push_back(data.value());
	}

	for (auto &datum : gpuData)
		constructTree(gpuTree, root, datum);

	return root;
}

TUXCLOCKER_PLUGIN_EXPORT(NvidiaPlugin)
