#pragma once

#include <Device.hpp>
#include <filesystem>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum PPTableType {
	Vega10,
	Navi,
	SMU13 // RDNA 3
};

struct VFPoint {
	int voltage;
	int clock;
};

struct AMDGPUData {
	// Full path, eg. /sys/class/drm/renderD128/device/hwmon
	std::string hwmonPath;
	amdgpu_device_handle devHandle;
	// Used as identifier
	std::string pciId;
	std::optional<PPTableType> ppTableType;
};

std::vector<std::string> pstateSectionLines(const std::string &header, const std::string &contents);

std::optional<TuxClocker::Device::Range<int>> parsePstateRangeLine(
    std::string title, const std::string &contents);

std::optional<TuxClocker::Device::Range<int>> parsePstateRangeLineWithRead(
    std::string title, AMDGPUData data);

std::optional<std::pair<int, int>> parseLineValuePair(const std::string &line);

std::optional<int> parseLineValue(const std::string &line);

std::optional<VFPoint> vfPoint(const std::string &section, int index, const std::string &table);

std::optional<VFPoint> vfPointWithRead(const std::string &section, int index, AMDGPUData data);

std::optional<PPTableType> fromPPTableContents(const std::string &contents);

std::optional<AMDGPUData> fromRenderDFile(const fs::directory_entry &entry);

std::vector<AMDGPUData> fromFilesystem();
