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

using namespace TuxClocker::Plugin;
using namespace TuxClocker::Device;
using namespace TuxClocker;

namespace fs = std::filesystem;

struct AMDAssignableInfo {
	// Thus is Linux only
	std::optional<std::string> hwmonPath;
	amdgpu_device_handle devHandle;
};

class AMDPlugin : public DevicePlugin {
public:
	AMDPlugin();
	std::optional<InitializationError> initializationError() {return std::nullopt;}
	void foo() {std::cout << "hi from AMD plugin\n";}
	TreeNode<DeviceNode> deviceRootNode();
	~AMDPlugin();
private:
	TreeNode<DeviceNode> m_rootNode;
	std::vector<AMDAssignableInfo> m_assignableInfoVec;
};

AMDPlugin::~AMDPlugin() {
	for (auto info : m_assignableInfoVec) {
		amdgpu_device_deinitialize(info.devHandle);
	}
}

AMDPlugin::AMDPlugin() {
	/*Assignable a([](auto arg) {
		return std::nullopt;
	});
	
	DeviceNode root{"AMD", a};
	m_rootNode.appendChild(root);*/
	
	struct FSInfo {
		std::string path;
		std::string filename;
	};
	
	std::vector<FSInfo> infoVec;
	// Iterate through files in GPU device folder and find which ones have amdgpu loaded
	for (const auto &entry : fs::directory_iterator(DRM_DIR_NAME)) {
		// Check if entry is renderD file
		if (entry.path().string().find(DRM_RENDER_MINOR_NAME) != std::string::npos) {
			infoVec.push_back(FSInfo{entry.path().string(), entry.path().filename().string()});
		}
	}
	
	for (const auto &info : infoVec) {
		auto fd = open(info.path.c_str(), O_RDONLY);
		auto v_ptr = drmGetVersion(fd);
		amdgpu_device_handle dev;
		uint32_t m, n;
		if (fd > 0 &&
		    v_ptr &&
		    amdgpu_device_initialize(fd, &m, &n, &dev) == 0 &&
		    std::string(v_ptr->name).find(_AMDGPU_NAME) != std::string::npos) {
				// Device uses amdgpu
				// Find hwmon path if available
				std::ostringstream stream;
				stream << "/sys/class/drm/" << info.filename << "/device/hwmon";
			
				std::optional<std::string> hwmonPath = std::nullopt;
				try {
					for (const auto &entry : fs::directory_iterator(stream.str())) {
						if (entry.path().filename().string().find("hwmon") != std::string::npos) {
							hwmonPath = entry.path().string();
							break;
						}
					}
				}
				// This exception is only abnormal to get on Linux
				catch (fs::filesystem_error &e) {}
				m_assignableInfoVec.push_back(AMDAssignableInfo{hwmonPath, dev});
		}
		close(fd);
		drmFreeVersion(v_ptr);
	}
}

TreeNode<DeviceNode> AMDPlugin::deviceRootNode() {
;
	return m_rootNode;
}

TUXCLOCKER_PLUGIN_EXPORT(AMDPlugin)
