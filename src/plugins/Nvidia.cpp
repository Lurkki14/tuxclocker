#include <Device.hpp>
#include <Plugin.hpp>

#include <fplus/fplus.hpp>
#include <iostream>
#include <patterns.hpp>
// Need to break alphabetic order since nvidia couldn't put the required includes in their header
#include <X11/Xlib.h> // Also breaks everything if it's in a different spot
#include <NVCtrl/NVCtrlLib.h>
#include <nvml.h>

using namespace TuxClocker;
using namespace TuxClocker::Device;
using namespace TuxClocker::Plugin;
using namespace mpark::patterns;

namespace fp = fplus;

template<typename T>
struct NvidiaGPU {
	std::string uuid;
	//std::string name;
	T identifier;
};

struct NvidiaGPUData {
	nvmlDevice_t devHandle;
	uint index;
};

template <typename T>
struct UnspecializedReadable {
	// This is specialized into a function with no arguments
	std::function<std::variant<ReadError, ReadableValue>(T)> func;
	std::optional<std::string> unit;
	std::string nodeName;
};

class NvidiaPlugin : public DevicePlugin {
public:
	NvidiaPlugin();
	std::optional<InitializationError> initializationError() {return std::nullopt;}
	TreeNode<DeviceNode> deviceRootNode() {return m_rootDeviceNode;}
	~NvidiaPlugin();
private:
	TreeNode<DeviceNode> m_rootDeviceNode;
	std::vector<nvmlDevice_t> m_devHandles;
	Display *m_dpy;
	template <typename In, typename Out>
	static std::variant<ReadError, ReadableValue> nvmlRead(nvmlDevice_t dev,
		std::function<nvmlReturn_t(nvmlDevice_t, In*)> readFunc,
		std::optional<std::function<Out(In)>> transformFunc = std::nullopt);
	static std::variant<ReadError, ReadableValue> nvctlRead(uint index,
		std::function<Bool(uint, int*)> readFunc);
};

template <typename In, typename Out>
std::variant<ReadError, ReadableValue> NvidiaPlugin::nvmlRead(nvmlDevice_t dev,
		std::function<nvmlReturn_t(nvmlDevice_t, In*)> readFunc,
		std::optional<std::function<Out(In)>> transformFunc) {
	In value;
	switch (readFunc(dev, &value)) {
		case NVML_SUCCESS: break;
		// Map to ReadError here
		default: return ReadError::UnknownError;
	}
	if (!transformFunc.has_value()) return value;
	return transformFunc.value()(value);
}

std::variant<ReadError, ReadableValue> NvidiaPlugin::nvctlRead(uint index,
		std::function<Bool(uint, int*)> readFunc) {
	int val;
	if (!readFunc(index, &val)) {
		// Get error from X error handler here
		return ReadError::UnknownError;
	}
	// All sensor readings through this make sense as uint
	return static_cast<uint>(val);
}

NvidiaPlugin::~NvidiaPlugin() {
	nvmlShutdown();
	if (m_dpy) XCloseDisplay(m_dpy);
}

NvidiaPlugin::NvidiaPlugin() : m_dpy() {
	nvmlInit();
	
	m_dpy = XOpenDisplay(NULL);
	
	// Make sure that features accessed through the different libraries are assigned to the correct GPUs
	std::vector<NvidiaGPU<uint>> nvctlGPUVec;
	int nvctrlGPUCount = 0;
	if (m_dpy && XNVCTRLQueryTargetCount(m_dpy, NV_CTRL_TARGET_TYPE_GPU, &nvctrlGPUCount)) {
		for (int i = 0; i < nvctrlGPUCount; i++) {
			char *uuid = NULL;
			if (XNVCTRLQueryTargetStringAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU, i, 0,
					NV_CTRL_STRING_GPU_UUID, &uuid)) {
				nvctlGPUVec.push_back({std::string(uuid), static_cast<uint>(i)});
				delete uuid;
			}
		}
	}
	
	std::vector<NvidiaGPU<nvmlDevice_t>> nvmlGPUVec;
	uint devCount = 0;
	if (nvmlDeviceGetCount_v2(&devCount) == NVML_SUCCESS) {
		for (uint i = 0; i < devCount; i++) {
			nvmlDevice_t dev;
			char uuid[NVML_DEVICE_NAME_BUFFER_SIZE];
			if (nvmlDeviceGetHandleByIndex_v2(i, &dev) == NVML_SUCCESS &&
					nvmlDeviceGetUUID(dev, uuid, NVML_DEVICE_UUID_BUFFER_SIZE) ==
					NVML_SUCCESS) {
				nvmlGPUVec.push_back({std::string(uuid), dev});
			}
		}
	}
		
	std::vector<UnspecializedReadable<uint>> rawNVCTRLNodes = {
		{
			[&](uint index) {
				return nvctlRead(index, [&](uint index, int *value) {
					return XNVCTRLQueryTargetAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU,
						index, 0, NV_CTRL_GPU_CURRENT_CORE_VOLTAGE, value);
				});
			},
			"mV",
			"Core Voltage"
		}
	};
	
	std::vector<UnspecializedReadable<nvmlDevice_t>> rawNVMLNodes = {
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, value);
				}, std::nullopt);
			},
			"°C",
			"Temperature"
		},
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, double>(dev, [](nvmlDevice_t dev,
						uint *value) {
					return nvmlDeviceGetPowerUsage(dev, value);
				}, [](uint val) {
					return static_cast<double>(val) / 1000;
				});
			},
			"W",
			"Power Usage"
		},
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetClockInfo(dev, NVML_CLOCK_GRAPHICS, value);
				}, std::nullopt);
			},
			"MHz",
			"Core Clock"
		},
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetClockInfo(dev, NVML_CLOCK_MEM, value);
				}, std::nullopt);
			},
			"MHz",
			"Memory Clock"
		}
	};
	
	
	struct NvidiaGPUDataOpt {
		std::string uuid;
		std::string name;
		std::optional<uint> index;
		std::optional<nvmlDevice_t> devHandle;
	};
	
	// Combine the GPU vectors into a single structure that can be iterated over
	// For both vectors: Find corresponding uuid from other vector, and append to combined vector if uuid doesn't exist in it
	std::vector<NvidiaGPUDataOpt> optDataVec;
	
	for (auto nvctlGPU : nvctlGPUVec) {
		// Should never contain more than one despite the name
		auto sameGPUs = fp::keep_if([nvctlGPU](auto nvmlGPU) {
				return nvmlGPU.uuid == nvctlGPU.uuid;
			}, nvmlGPUVec);
		
		auto dev =
			(!sameGPUs.empty()) ? std::optional(sameGPUs[0].identifier) : std::nullopt;
		optDataVec.push_back({.uuid = nvctlGPU.uuid,
			.index = nvctlGPU.identifier,
			.devHandle = dev
		});
	}
	
	for (auto nvmlGPU : nvmlGPUVec) {
		bool alreadyInVec = !fp::keep_if([nvmlGPU](auto nvOpt) {
				return nvmlGPU.uuid == nvOpt.uuid;
			}, optDataVec).empty();
		if (alreadyInVec) continue;
		
		auto sameGPUs = fp::keep_if([nvmlGPU](auto nvctlGPU) {
				return nvmlGPU.uuid == nvctlGPU.uuid;
			}, nvctlGPUVec);
		auto index = 
			(!sameGPUs.empty()) ? std::optional(sameGPUs[0].identifier) : std::nullopt;
		optDataVec.push_back({.uuid = nvmlGPU.uuid,
			.index = index,
			.devHandle = nvmlGPU.identifier
		});
	}
	
	// Get GPU names
	uint gi = 0; // Use GPU index as name if getting name fails
	for (auto &nvOpt : optDataVec) {
		std::optional<std::string> gpuName = std::nullopt;
		match(nvOpt.devHandle)(pattern(some(arg)) = [rawNVMLNodes, &gpuName](auto dev) {
			// Try to get GPU name from NVML
			char name[NVML_DEVICE_NAME_BUFFER_SIZE];
			if (nvmlDeviceGetName(dev, name, NVML_DEVICE_NAME_BUFFER_SIZE) == NVML_SUCCESS)
				gpuName = std::string(name);
			
		}, pattern(_) = []{});
		
		match(nvOpt.index)(pattern(some(arg)) = [&](auto index) {
			char *name = nullptr;
			if (XNVCTRLQueryStringAttribute(m_dpy, 0, index,
					NV_CTRL_STRING_PRODUCT_NAME, &name)) {
				gpuName = std::string(name);
				delete name;
			}
		}, pattern(_) = []{});
		
		// Name couldn't be gotten from either lib
		char buf[7];
		if (!gpuName.has_value()) {
			// std::stringstream is as ugly while being twice as slow
			snprintf(buf, 7, "GPU %u", gi);
			gpuName = std::string(buf);
		}
		nvOpt.name = gpuName.value();
		gi++;
	};
	
	
	
	// [GPUOpt] -> [TreeNode DeviceNode]
	// Map GPU data to readables
	TreeNode<DeviceNode> rootNode;
	auto gpuNodes = fp::transform([&](auto nvOpt) {
			TreeNode<DeviceNode> gpuRoot = TreeNode(DeviceNode{.name = nvOpt.name});
			
			// Get nvml nodes if there is a device
			if_let(pattern(some(arg)) = nvOpt.devHandle) = [&](auto dev) {
				auto availableNVMLNodes = fp::keep_if([dev](auto rawNode) {
						auto result = rawNode.func(dev);
						match(result)(pattern(as<ReadError>(_)) = []() {return false;},
								pattern(_) = []() {return true;});
						return true;
					}, rawNVMLNodes);
				auto specializedNVMLNodes = fp::transform([&](auto rawNode) {
						auto readable = DynamicReadable(
							[&]() {return rawNode.func(dev);});
						auto devNode = DeviceNode{.name = rawNode.nodeName,
							.interface = readable
						};
						return devNode;
					}, availableNVMLNodes);
				for (auto &specNode : specializedNVMLNodes) {
					gpuRoot.appendChild(specNode);
				}
			};
			
			// Same for NVCtrl
			if_let(pattern(some(arg)) = nvOpt.index) = [&](auto index) {
				auto availableNodes = fp::keep_if([index](auto rawNode) {
						auto result = rawNode.func(index);
						match(result)(pattern(as<ReadError>(_)) = []() {return false;},
								pattern(_) = []() {return true;});
						return true;
					}, rawNVCTRLNodes);
				auto specializedNodes = fp::transform([index](auto rawNode) {
						auto readable = DynamicReadable(
							[&]() {return rawNode.func(index);});
						auto devNode = DeviceNode{.name = rawNode.nodeName,
							.interface = readable
						};
						return devNode;						
					}, availableNodes);
				for (auto &specNode : specializedNodes) gpuRoot.appendChild(specNode);
			};
			
			return gpuRoot;
		}, optDataVec);
	for (auto &gpuNode : gpuNodes) rootNode.appendChild(gpuNode);
	
	m_rootDeviceNode = rootNode;
	/*std::cout << nvctrlGPUCount << "GPUs\n";
	for (auto nvmlGPU : nvmlGPUVec) {
		auto res = rawNodes[0].func(NvidiaGPUData{nvmlGPU.identifier, 0});
		if (auto vp = std::get_if<ReadableValue>(&res)) {
			if (auto up = std::get_if<uint>(vp)) {
				std::cout << *up << "\n";
			}
		}
	}*/
}

TUXCLOCKER_PLUGIN_EXPORT(NvidiaPlugin)
