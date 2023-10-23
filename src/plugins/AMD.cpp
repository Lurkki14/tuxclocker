#include <Crypto.hpp>
#include <patterns.hpp>
#include <Plugin.hpp>
#include <TreeConstructor.hpp>
#include <Utils.hpp>

#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <functional>
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

struct AMDGPUData {
	// Full path, eg. /sys/class/drm/renderD128/device/hwmon
	std::string hwmonPath;
	amdgpu_device_handle devHandle;
	// Used as identifier
	std::string pciId;
};

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

		drmFreeVersion(v_ptr);
		return AMDGPUData{
		    .hwmonPath = hwmonPath.value(),
		    .devHandle = dev,
		    .pciId = std::to_string(info.device_id),
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
		{getTemperature, {}}
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
