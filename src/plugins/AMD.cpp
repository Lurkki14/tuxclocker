#include <Crypto.hpp>
#include <patterns.hpp>
#include <Plugin.hpp>
#include <TreeConstructor.hpp>
#include <Utils.hpp>

#include <cmath>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
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

		// We don't handle 0 (no fan control at all)
		auto value = static_cast<uint>(std::stoi(*string));
		if (value == 0)
			return std::nullopt;
		return value;
	};

	auto setFunc = [=](AssignmentArgument a) -> std::optional<AssignmentError> {
		if (!std::holds_alternative<uint>(a))
			return AssignmentError::InvalidType;

		auto value = std::get<uint>(a);
		if (!hasEnum(value, enumVec))
			return AssignmentError::OutOfRange;

		if (std::ofstream{path} << value)
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
		// TODO: is this microwatts too?
		if (amdgpu_query_sensor_info(data.devHandle, AMDGPU_INFO_SENSOR_GPU_AVG_POWER,
			sizeof(power), &power) == 0)
			return static_cast<double>(power) / 1000000;
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
