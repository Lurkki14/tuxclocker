#include <Functional.hpp>
#include <patterns.hpp>
#include <Plugin.hpp>

#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <unistd.h>
#include <xf86drm.h>

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
using namespace TuxClocker::Device;
using namespace TuxClocker;

namespace fs = std::filesystem;

struct AMDGPUInfo {
	// Thus is Linux only
	std::optional<std::string> hwmonPath;
	amdgpu_device_handle devHandle;
};

class AMDPlugin : public DevicePlugin {
public:
	AMDPlugin();
	std::optional<InitializationError> initializationError() { return std::nullopt; }
	void foo() { std::cout << "hi from AMD plugin\n"; }
	TreeNode<DeviceNode> deviceRootNode();
	~AMDPlugin();
private:
	TreeNode<DeviceNode> m_rootNode;
	std::vector<AMDGPUInfo> m_GPUInfoVec;
	template <typename T>
	static std::variant<ReadError, ReadableValue> libdrmRead(amdgpu_device_handle dev,
	    uint query, std::optional<std::function<T(T)>> transformFunc = std::nullopt);
};

AMDPlugin::~AMDPlugin() {
	for (auto info : m_GPUInfoVec) {
		amdgpu_device_deinitialize(info.devHandle);
	}
}

template <typename T>
std::variant<ReadError, ReadableValue> AMDPlugin::libdrmRead(
    amdgpu_device_handle dev, uint query, std::optional<std::function<T(T)>> transformFunc) {
	T value;
	auto retval = amdgpu_query_sensor_info(dev, query, sizeof(T), &value);
	if (retval != 0)
		return ReadError::UnknownError;

	if (!transformFunc.has_value())
		return value;
	return transformFunc.value()(value);
}

AMDPlugin::AMDPlugin() {
	struct FSInfo {
		std::string path;
		std::string filename;
	};

	std::vector<FSInfo> infoVec;
	// Iterate through files in GPU device folder and find which ones have amdgpu loaded
	for (const auto &entry : fs::directory_iterator(DRM_DIR_NAME)) {
		// Check if entry is renderD file
		if (entry.path().string().find(DRM_RENDER_MINOR_NAME) != std::string::npos) {
			infoVec.push_back(
			    FSInfo{entry.path().string(), entry.path().filename().string()});
		}
	}

	for (const auto &info : infoVec) {
		auto fd = open(info.path.c_str(), O_RDONLY);
		auto v_ptr = drmGetVersion(fd);
		amdgpu_device_handle dev;
		uint32_t m, n;
		int devInitRetval;
		if (fd > 0 && v_ptr &&
		    std::string(v_ptr->name).find(_AMDGPU_NAME) != std::string::npos &&
		    (devInitRetval = amdgpu_device_initialize(fd, &m, &n, &dev)) == 0) {
			// Device uses amdgpu
			// Find hwmon path if available
			std::ostringstream stream;
			stream << "/sys/class/drm/" << info.filename << "/device/hwmon";

			std::optional<std::string> hwmonPath = std::nullopt;
			try {
				for (const auto &entry : fs::directory_iterator(stream.str())) {
					if (entry.path().filename().string().find("hwmon") !=
					    std::string::npos) {
						hwmonPath = entry.path().string();
						break;
					}
				}
			}
			// This exception is only abnormal to get on Linux
			catch (fs::filesystem_error &e) {
			}
			m_GPUInfoVec.push_back(AMDGPUInfo{hwmonPath, dev});
			continue;
		}
		// if (devInitRetval == 0) amdgpu_device_deinitialize(dev);
		close(fd);
		drmFreeVersion(v_ptr);
	}

	// Add nodes with successful queries to device node

	// Data structure for mapping to DynamicReadables
	struct UnspecializedReadable {
		std::function<std::variant<ReadError, ReadableValue>(AMDGPUInfo)> func;
		std::optional<std::string> unit;
		std::string nodeName;
	};

	// List of query functions for DynamicReadables
	std::vector<UnspecializedReadable> rawNodes = {
	    {[&](AMDGPUInfo info) {
		     return libdrmRead<uint>(info.devHandle, AMDGPU_INFO_SENSOR_GPU_TEMP,
			 std::function<uint(uint)>([](uint val) { return val / 1000; }));
	     },
		std::nullopt, "Temperature"}};

	for (auto info : m_GPUInfoVec) {
		auto availableReadables = filter(rawNodes,
		    std::function<bool(UnspecializedReadable)>([info](UnspecializedReadable ur) {
			    auto result = ur.func(info);
			    // Check if this node returns an error or not
			    match(result)(
				pattern(as<ReadError>(_)) = [] { return false; },
				pattern(_) = [] { return true; });
			    return true;
		    }));
		auto specializedNodes = map(availableReadables,
		    std::function<DynamicReadable(UnspecializedReadable)>(
			[info](UnspecializedReadable ur) {
				// auto specializedFunc = specializeFunction<std::variant<ReadError,
				// ReadableValue>, AMDGPUInfo>(info, ur.func);
				return DynamicReadable([info, ur]() { return ur.func(info); });
			}));
	}
}

TreeNode<DeviceNode> AMDPlugin::deviceRootNode() { return m_rootNode; }

TUXCLOCKER_PLUGIN_EXPORT(AMDPlugin)
