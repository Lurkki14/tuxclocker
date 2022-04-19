#include <Crypto.hpp>
#include <Device.hpp>
#include <NVCtrl/NVCtrl.h>
#include <Plugin.hpp>

#include <fplus/fplus.hpp>
#include <iostream>
#include <optional>
#include <patterns.hpp>
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
	// One time function for getting a hash
	std::function<std::string(std::string, T)> hash;
	
	static std::vector<DeviceNode> toDeviceNodes(
			std::vector<UnspecializedReadable<T>> rawNodes, T data, std::string uuid) {
		std::vector<DeviceNode> retval;
		
		for (const auto &rawNode : rawNodes) {
			// Check if value can be read
			if_let(pattern(as<ReadableValue>(_)) = rawNode.func(data)) = [=, &retval] {
				auto readable = DynamicReadable([=] {
					return rawNode.func(data);
				}, rawNode.unit);
				auto devNode = DeviceNode{
					.name = rawNode.nodeName,
					.interface = readable,
					.hash = rawNode.hash(uuid, data)
				};
				retval.push_back(devNode);
			};
		}
		return retval;
	}
	// Create StaticReadable nodes
	static std::vector<DeviceNode> staticToDeviceNodes(
			std::vector<UnspecializedReadable<T>> rawNodes, T data, std::string uuid) {
		std::vector<DeviceNode> retval;
		
		for (const auto &rawNode : rawNodes) {
			if_let(pattern(as<ReadableValue>(arg)) = rawNode.func(data)) =
					[=, &retval] (ReadableValue value) {
				auto staticReadable = StaticReadable(
					value, rawNode.unit
				);
				auto devNode = DeviceNode{
					.name = rawNode.nodeName,
					.interface = staticReadable,
					.hash = rawNode.hash(uuid, data)
				};
				retval.push_back(devNode);
			};
		}
		return retval;
	}
		
};

template <typename T>
struct UnspecializedAssignable {
	// Method to get the AssignableInfo since they differ per GPU.
	std::function<std::optional<AssignableInfo>(T)> assignableInfo;
	std::function<std::optional<AssignmentError>(T, AssignableInfo, AssignmentArgument)> func;
	std::function<std::optional<AssignmentArgument>(T)> currentValueFunc;
	std::optional<std::string> unit;
	std::string nodeName;
	std::function<std::string(std::string, T)> hash;
	
	static std::vector<DeviceNode> toDeviceNodes(
			std::vector<UnspecializedAssignable<T>> rawNodes,
			T devData, std::string uuid) {
		std::vector<DeviceNode> retval;
		for (auto &rawNode : rawNodes) {
			if_let(pattern(some(arg)) = rawNode.assignableInfo(devData)) = [&](auto info) {
				// Create an individual Assignable from a generalized Assignable
				auto assignable = Assignable([=](auto arg) {
					return rawNode.func(devData, info, arg);
				}, 
				info,
				[=]() {
					return rawNode.currentValueFunc(devData);
				});
				auto node = DeviceNode {
					.name = rawNode.nodeName,
					.interface = assignable,
					.hash = rawNode.hash(uuid, devData)
				};
				retval.push_back(node);
			};
		}
		return retval;
	}
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
	template <typename Out>
	static std::variant<ReadError, ReadableValue> nvctlRead(uint index,
		std::function<Bool(uint, int*)> readFunc,
		std::optional<std::function<Out(int)>> transformFunc = std::nullopt);
	std::optional<AssignmentError> nvctlWrite(uint index, int mask,
		int target, int attribute, int value);
	uint nvctrlPerfModes(uint index);
	static uint nvmlFanCount(nvmlDevice_t dev);
	int nvctrlFanCount(uint index);
	static ReadError fromNVMLError(nvmlReturn_t err);
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
	// This is useless, just take a function of nvmlDevice_t -> a -> ReadableValue
	// Because this isn't compile time we can't use nvmlUtilization_t 
	if (!transformFunc.has_value()) return value;
	return transformFunc.value()(value);
}

ReadError NvidiaPlugin::fromNVMLError(nvmlReturn_t err) {
	// TODO: add conversions
	switch (err) {
		default:
			return ReadError::UnknownError;
	}
	return ReadError::UnknownError;
}

template <typename Out>
std::variant<ReadError, ReadableValue> NvidiaPlugin::nvctlRead(uint index,
		std::function<Bool(uint, int*)> readFunc,
		std::optional<std::function<Out(int)>> transformFunc) {
	int val;
	if (!readFunc(index, &val)) {
		// TODO: Get error from X error handler here
		return ReadError::UnknownError;
	}
	if (transformFunc.has_value())
		return transformFunc.value()(val);
	return static_cast<Out>(val);
}

std::optional<AssignmentError> NvidiaPlugin::nvctlWrite(uint index,
		int mask, int target, int attribute, int value) {
	if (XNVCTRLSetTargetAttributeAndGetStatus(m_dpy, target, index,
			mask, attribute, value)) {
		// Success
		return std::nullopt;
	}
	return AssignmentError::UnknownError;
	// Get error from X error handler
}

uint NvidiaPlugin::nvctrlPerfModes(uint index) {
	// Thanks to Artifth for original code
	char *result;
	if (XNVCTRLQueryTargetStringAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU,
			index, 0, NV_CTRL_STRING_PERFORMANCE_MODES, &result)) {
		auto s = std::string(result);
		auto modes = std::count(s.begin(), s.end(), ';');
		delete result;
		return modes; 
	}
	// Usually there's 3 perf modes
	return 3;
}

uint NvidiaPlugin::nvmlFanCount(nvmlDevice_t dev) {
	const int maxFans = 10;
	uint i = 0;
	uint speed;
	// Check for which fan indices we get a fan speed reading
	for (; i < maxFans; i++) {
		if (nvmlDeviceGetFanSpeed_v2(dev, i, &speed) != NVML_SUCCESS)
			break;
	}
	return i;
}

int NvidiaPlugin::nvctrlFanCount(uint index) {
	unsigned char *data;
	int data_len;

	if (!XNVCTRLQueryTargetBinaryData(m_dpy, NV_CTRL_TARGET_TYPE_GPU,
			index, 0, NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
			&data, &data_len)) {
		return 0;           
	}
	auto retval = static_cast<int>(*data);
	delete data;
	return retval;
}

int nvctrlFanCount(uint index);

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
				return nvctlRead<double>(index, [&](uint index, int *value) {
					return XNVCTRLQueryTargetAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU,
						index, 0, NV_CTRL_GPU_CURRENT_CORE_VOLTAGE, value);
				}, [](int value) {
					return static_cast<double>(value) / 1000;
				});
			},
			"mV",
			"Core Voltage",
			[](std::string uuid, uint) {
				return md5(uuid + "Core Voltage");
			}
		}
	};
	
	std::vector<UnspecializedReadable<nvmlDevice_t>> rawNVMLNodes = {
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
			"Power Usage",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Power Usage");
			}
		},
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetClockInfo(dev, NVML_CLOCK_GRAPHICS, value);
				}, std::nullopt);
			},
			"MHz",
			"Core Clock",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Core Clock");
			}
		},
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetClockInfo(dev, NVML_CLOCK_MEM, value);
				}, std::nullopt);
			},
			"MHz",
			"Memory Clock",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Memory Clock");
			}
		}
	};

	std::vector<UnspecializedReadable<nvmlDevice_t>> rawNVMLUtilNodes = {
		{
			[](nvmlDevice_t dev) -> ReadResult {
				nvmlUtilization_t value;
				nvmlReturn_t err;
				if ((err = nvmlDeviceGetUtilizationRates(dev, &value)) != NVML_SUCCESS)
					return fromNVMLError(err);
				return value.gpu;
			},
			"%",
			"Core Utilization",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Core Utilization");
			}
		},
		{
			[](nvmlDevice_t dev) -> ReadResult {
				nvmlUtilization_t value;
				nvmlReturn_t err;
				if ((err = nvmlDeviceGetUtilizationRates(dev, &value)) != NVML_SUCCESS)
					return fromNVMLError(err);
				return value.memory;
			},
			"%",
			"Memory Utilization",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Memory Utilization");
			}
		}
	};


	std::vector<UnspecializedReadable<uint>> rawNVCTRLUtilNodes = {
		{
			[&](uint index) -> ReadResult {
				const std::string target("PCIe=");
				char *val;
				// We're looking for a format of PCIe=xxx
				if (!XNVCTRLQueryTargetStringAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU, index, 0,
						NV_CTRL_STRING_GPU_UTILIZATION, &val)) {
					return ReadError::UnknownError;
				}
				// The index after the '=', rest of the string should be the number
				auto strIndex = std::string(val).find(target) + target.length();
				auto retval =  static_cast<uint>(std::stoul(std::string(val).substr(strIndex)));
				delete val;
				return retval;
			},
			"%",
			"PCIe Bandwidth Utilization",
			[](std::string uuid, uint) {
				return md5(uuid + "PCIe Bandwidth Utilization");
			}
		}
	};

	std::vector<UnspecializedAssignable<nvmlDevice_t>> rawNVMLAssignables = {
		{
			[](nvmlDevice_t dev) -> std::optional<AssignableInfo> {
				uint min, max;
				if (nvmlDeviceGetPowerManagementLimitConstraints(dev, &min, &max) !=
						NVML_SUCCESS)
					return std::nullopt;
				// mW -> W
				Range r(static_cast<double>(min / 1000), static_cast<double>(max / 1000));
				return r;
			},
			[](nvmlDevice_t dev, AssignableInfo info, AssignmentArgument a_arg)
					-> std::optional<AssignmentError> {
				std::optional<AssignmentError> retval = AssignmentError::InvalidType;
				if_let(pattern(as<double>(arg)) = a_arg) = [=, &retval](auto val) {
					/* TODO: this is an unsafe conversion, either add a template parameter for
						* range, check the type here or handle the possible exception. */
					auto ri = std::get<RangeInfo>(info);
					auto range = std::get<Range<double>>(ri);
					if (range.min > val || range.max < val)
						retval = AssignmentError::OutOfRange;
					// W -> mW
					switch (nvmlDeviceSetPowerManagementLimit(dev, round(val * 1000))) {
						case NVML_SUCCESS: retval = std::nullopt; break;
						case NVML_ERROR_NO_PERMISSION: retval = AssignmentError::NoPermission; break;
						default: retval = AssignmentError::UnknownError; break;
					}
				};
				return retval;
			},
			[](nvmlDevice_t dev) -> std::optional<AssignmentArgument> {
				uint limit;
				auto retval = nvmlDeviceGetPowerManagementLimit(dev, &limit);
				if (retval != NVML_SUCCESS)
					return std::nullopt;
				return static_cast<double>(limit) / 1000;
				// In correspondance with Range<double>
			},
			"W",
			"Power Limit",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Power Limit");
			}
		}
	};
	
	struct NVClockInfo {
		uint maxState;
		uint index;
	};
	
	std::vector<UnspecializedAssignable<NVClockInfo>> rawNVCTRLClockAssignables = {
		{
			[=](NVClockInfo info) -> std::optional<AssignableInfo> {
				NVCTRLAttributeValidValuesRec values;
				if (XNVCTRLQueryValidTargetAttributeValues(m_dpy, NV_CTRL_TARGET_TYPE_GPU, info.index, 
						info.maxState, NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET, &values)) {
					// Transfer rate -> clock speed
					Range<int> r(values.u.range.min / 2, values.u.range.max / 2);
					return r;
				}
				return std::nullopt;
			},
			[=](NVClockInfo c_info, AssignableInfo info, AssignmentArgument a_arg) {
				std::optional<AssignmentError> retval = AssignmentError::InvalidType;
				if_let(pattern(as<int>(arg)) = a_arg) = [=, &retval](auto val){
					auto ri = std::get<RangeInfo>(info);
					auto range = std::get<Range<int>>(ri);
					if (range.min > val || range.max < val)
						retval = AssignmentError::OutOfRange;
					// MHz -> transfer rate
					else
						retval =  nvctlWrite(c_info.index, c_info.maxState, 
							NV_CTRL_TARGET_TYPE_GPU,
							NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET, val * 2);
				};
				return retval;
			},
			[=](NVClockInfo c_info) -> std::optional<AssignmentArgument> {
				int value;
				auto valid = XNVCTRLQueryTargetAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU, c_info.index,
						c_info.maxState, NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET, &value);
				if (valid)
					return value / 2;
				return std::nullopt;
			},
			"MHz",
			"Memory Clock Offset",
			[](std::string uuid, NVClockInfo) {
				return md5(uuid + "Memory Clock Offset");
			}
		}
	};
	
	std::vector<UnspecializedAssignable<uint>> rawNVCTRLAssignables = {
		{
			[=](uint index) -> std::optional<AssignableInfo> {
				NVCTRLAttributeValidValuesRec values;
				if (XNVCTRLQueryValidTargetAttributeValues(m_dpy, NV_CTRL_TARGET_TYPE_GPU,
						index, 0,
						NV_CTRL_GPU_COOLER_MANUAL_CONTROL, &values) &&
						(values.permissions & ATTRIBUTE_TYPE_WRITE)) {
					std::vector<Enumeration> opts = {{"Automatic", 0}, {"Manual", 1}};
					return opts;
				}
				return std::nullopt;
			},
			[=](uint index, AssignableInfo, AssignmentArgument a_arg) {
				std::optional<AssignmentError> ret = AssignmentError::InvalidType;
				if_let(pattern(as<uint>(arg)) = a_arg) = [=, &ret](auto u) {
					switch(u) {
						case 0:
							ret = nvctlWrite(index, 0, NV_CTRL_TARGET_TYPE_GPU,
								NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
								NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE);
							break;
						case 1:
							ret = nvctlWrite(index, 0, NV_CTRL_TARGET_TYPE_GPU,
								NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
								NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE);
							break;
						default: ret = AssignmentError::OutOfRange;
					}
				};
				return ret;
			},
			[=](uint index) -> std::optional<AssignmentArgument> {
				int value;
				auto valid = XNVCTRLQueryTargetAttribute(m_dpy, NV_CTRL_TARGET_TYPE_GPU, index, 0,
					NV_CTRL_GPU_COOLER_MANUAL_CONTROL, &value);
				if (valid)
					return (value == NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE) ?
						0 : 1;
				return std::nullopt;
			},
			std::nullopt,
			"Fan Mode",
			[](std::string uuid, uint) {
				return md5(uuid + "Fan Mode");
			}
		}
	};
	
	
	struct NVMLFanInfo {
		nvmlDevice_t dev;
		uint index;
	};
	
	std::vector<UnspecializedReadable<NVMLFanInfo>> rawNVMLFanReadables = {
		{
			[](NVMLFanInfo info) {
				return nvmlRead<uint, uint>(info.dev, [info](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetFanSpeed_v2(dev, info.index, value);
				}, std::nullopt);
			},
			"%",
			"Fan Speed",
			[](std::string uuid, NVMLFanInfo info) {
				return md5(uuid + "Fan Speed" + std::to_string(info.index));
			}
		}
	};
	
	std::vector<UnspecializedReadable<nvmlDevice_t>> rawNVMLTempInfoNodes = {
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetTemperatureThreshold(dev,
							NVML_TEMPERATURE_THRESHOLD_SHUTDOWN, value);
				}, std::nullopt);
			},
			"C",
			"Shutdown Temperature",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Shutdown Temperature");
			}
		},
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetTemperatureThreshold(dev,
							NVML_TEMPERATURE_THRESHOLD_SLOWDOWN, value);
				}, std::nullopt);
			},
			"C",
			"Slowdown Temperature",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Slowdown Temperature");
			}
		}
	};
	
	std::vector<UnspecializedReadable<nvmlDevice_t>> rawNVMLTempNodes = {
		{
			[](nvmlDevice_t dev) {
				return nvmlRead<uint, uint>(dev, [](nvmlDevice_t dev, uint *value) {
					return nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, value);
				}, std::nullopt);
			},
			"°C",
			"Temperature",
			[](std::string uuid, nvmlDevice_t) {
				return md5(uuid + "Temperature");
			}
		}
	};
	
	struct NVCtrlFanInfo {
		uint gpuIndex;
		int fanIndex;
	};
	
	std::vector<UnspecializedAssignable<NVCtrlFanInfo>> rawNVCTRLFanAssignables = {
		{
			[=](NVCtrlFanInfo info) -> std::optional<AssignableInfo> {
				NVCTRLAttributeValidValuesRec values;
				if (XNVCTRLQueryValidTargetAttributeValues(m_dpy, NV_CTRL_TARGET_TYPE_GPU,
						info.gpuIndex, info.fanIndex,
						NV_CTRL_GPU_COOLER_MANUAL_CONTROL, &values) &&
						(values.permissions & ATTRIBUTE_TYPE_WRITE)) {
					return Range<int>(0, 100);
				}
				return std::nullopt;
			},
			[=](NVCtrlFanInfo info, AssignableInfo, AssignmentArgument a_arg) {
				std::optional<AssignmentError> retval = AssignmentError::InvalidType;
				if_let(pattern(as<int>(arg)) = a_arg) = [=, &retval](auto i) {
					if (i < 0 || i > 100)
						retval = AssignmentError::OutOfRange;
					else
						retval = nvctlWrite(info.gpuIndex, info.fanIndex,
							NV_CTRL_TARGET_TYPE_COOLER,
							NV_CTRL_THERMAL_COOLER_LEVEL, i);
				};
				
				return retval;
			},
			[=](NVCtrlFanInfo info) -> std::optional<AssignmentArgument> {
				int value;
				auto valid = XNVCTRLQueryTargetAttribute(m_dpy, NV_CTRL_TARGET_TYPE_COOLER,
					info.gpuIndex, info.fanIndex,
					NV_CTRL_THERMAL_COOLER_LEVEL, &value);
				if (valid)
					return value;
				return std::nullopt;
			},
			"%",
			"Fan Speed",
			[](std::string uuid, NVCtrlFanInfo info) {
				return md5(uuid + "Fan Speed Write" + std::to_string(info.fanIndex));
			}
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
	// Map GPU data to device nodes
	TreeNode<DeviceNode> rootNode;
	auto gpuNodes = fp::transform([&](auto nvOpt) {
		TreeNode<DeviceNode> gpuRoot = TreeNode(DeviceNode{
			.name = nvOpt.name,
			.hash = md5(nvOpt.uuid)
		});
	

		// Has nodes from NVML and NVCtrl
		auto utilRoot = TreeNode(DeviceNode {
			.name = "Utilizations",
			.hash = md5(nvOpt.uuid + "Utilizations")
		});

		std::vector<DeviceNode> nvmlFanNodes;
		// Get nvml nodes if there is a device
		if_let(pattern(some(arg)) = nvOpt.devHandle) = [&](auto dev) {
			for (auto &node : UnspecializedReadable<nvmlDevice_t>::toDeviceNodes(
					rawNVMLNodes, dev, nvOpt.uuid))
				gpuRoot.appendChild(node);
			
			for (auto &node : UnspecializedAssignable<nvmlDevice_t>::toDeviceNodes(
						rawNVMLAssignables, dev, nvOpt.uuid))
				gpuRoot.appendChild(node);
			
			for (uint i = 0; i < nvmlFanCount(dev); i++) {
				NVMLFanInfo info{dev, i};
				for (auto &node : UnspecializedReadable<NVMLFanInfo>::toDeviceNodes(
						rawNVMLFanReadables, info, nvOpt.uuid))
					nvmlFanNodes.push_back(node);
			}

			for (auto &node : UnspecializedReadable<nvmlDevice_t>::toDeviceNodes(
					rawNVMLUtilNodes, dev, nvOpt.uuid))
				utilRoot.appendChild(node);

			auto tempInfoRoot = TreeNode(DeviceNode{
				.name = "Temperatures",
				.hash = md5(nvOpt.uuid + "Temperatures")
			});
			auto tempNodes = UnspecializedReadable<nvmlDevice_t>::toDeviceNodes(
				rawNVMLTempNodes, dev, nvOpt.uuid);
			
			auto tempInfoNodes = UnspecializedReadable<nvmlDevice_t>::staticToDeviceNodes(
				rawNVMLTempInfoNodes, dev, nvOpt.uuid);
			
			if (!tempNodes.empty() || !tempInfoNodes.empty()) { 
				for (auto &infoNode : tempInfoNodes)
					tempInfoRoot.appendChild(infoNode);
				for (auto &tempNode : tempNodes)
					tempInfoRoot.appendChild(tempNode);
				
				gpuRoot.appendChild(tempInfoRoot);
			}
		};
		
		std::vector<DeviceNode> nvctrlFanNodes;
		// Same for NVCtrl
		if_let(pattern(some(arg)) = nvOpt.index) = [&](auto index) {
			for (auto &node : UnspecializedReadable<uint>::toDeviceNodes(rawNVCTRLNodes, index,
					nvOpt.uuid))
				gpuRoot.appendChild(node);
								
			auto clockInfo = NVClockInfo{nvctrlPerfModes(index), index};
			for (auto &node : UnspecializedAssignable<NVClockInfo>::toDeviceNodes(
					rawNVCTRLClockAssignables, clockInfo, nvOpt.uuid))
				gpuRoot.appendChild(node);
			
			for (auto &node : UnspecializedAssignable<uint>::toDeviceNodes(
					rawNVCTRLAssignables, index, nvOpt.uuid))
				gpuRoot.appendChild(node);
			
			for (int i = 0; i < nvctrlFanCount(index); i++) {
				NVCtrlFanInfo info{index, i};
				for (auto &node : UnspecializedAssignable<NVCtrlFanInfo>::toDeviceNodes(
						rawNVCTRLFanAssignables, info, nvOpt.uuid))
					nvctrlFanNodes.push_back(node);
			}
			for (auto &node : UnspecializedReadable<uint>::toDeviceNodes(
					rawNVCTRLUtilNodes, index, nvOpt.uuid))
				utilRoot.appendChild(node);
		};
	
		if (!utilRoot.children().empty())
			gpuRoot.appendChild(utilRoot);

		TreeNode<DeviceNode> fanRoot = TreeNode(DeviceNode{
			.name = "Fans",
			.hash = md5(nvOpt.uuid + "Fans")
		});
		// Use a common subparent for readable and assignable fan nodes, if n > 1
		auto maxFanCount = std::max(nvmlFanNodes.size(), nvctrlFanNodes.size());
		if (1) {
			for (uint i = 0; i < maxFanCount; i++) {
				auto subFanRoot = TreeNode<DeviceNode>(DeviceNode{
					.name = std::to_string(i),
					.hash = md5(nvOpt.uuid + "Fan" + std::to_string(i))
				});
				if (i < nvctrlFanNodes.size())
					subFanRoot.appendChild(nvctrlFanNodes[i]);
				if (i < nvmlFanNodes.size())
					subFanRoot.appendChild(nvmlFanNodes[i]);
				fanRoot.appendChild(subFanRoot);
			}
			gpuRoot.appendChild(fanRoot);
		}
		else {
			// Add as direct children to GPU
			for (auto &node : nvmlFanNodes)
				gpuRoot.appendChild(node);
			
			for (auto &node : nvctrlFanNodes)
				gpuRoot.appendChild(node);
		}
		
		return gpuRoot;
	}, optDataVec);
	
	for (auto &gpuNode : gpuNodes) rootNode.appendChild(gpuNode);
	m_rootDeviceNode = rootNode;
}

TUXCLOCKER_PLUGIN_EXPORT(NvidiaPlugin)
