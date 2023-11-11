#include "AMDUtils.hpp"

#include <fcntl.h>
#include <fplus/fplus.hpp>
#include <unistd.h>
#include <Utils.hpp>
#include <xf86drm.h>

#define _AMDGPU_NAME "amdgpu"

using namespace TuxClocker::Device;

std::vector<std::string> pstateSectionLines(
    const std::string &header, const std::string &contents) {
	std::vector<std::string> retval;

	auto isNewline = [](char c) { return c == '\n'; };
	auto lines = fplus::split_by(isNewline, false, contents);

	int startIndex = -1;
	for (int i = 0; i < lines.size(); i++) {
		// Find section start
		if (lines[i].find(header) != std::string::npos) {
			startIndex = i + 1;
			break;
		}
	}
	if (startIndex == -1)
		return {};

	for (int i = startIndex; i < lines.size(); i++) {
		if (isdigit(lines[i].at(0))) {
			// We're still in the section
			retval.push_back(lines[i]);
		} else
			// Line doesn't start with digit, another section has started
			break;
	}
	return retval;
}

std::vector<std::string> pstateSectionLinesWithRead(const std::string &header, AMDGPUData data) {
	auto contents = fileContents(data.devPath + "/pp_od_clk_voltage");
	if (!contents.has_value())
		return {};

	return pstateSectionLines(header, *contents);
}

std::optional<Range<int>> parsePstateRangeLine(std::string title, const std::string &contents) {
	// For example:
	// MCLK:     625Mhz        930Mhz
	auto isNewline = [](char c) { return c == '\n'; };
	auto lines = fplus::split_by(isNewline, false, contents);

	for (auto &line : lines) {
		if (line.rfind(title, 0) == 0) {
			// Line starts with title
			// Only split on whitespace
			auto words = fplus::split_one_of(std::string{" "}, false, line);
			if (words.size() >= 3)
				return Range<int>{std::stoi(words[1]), std::stoi(words[2])};
		}
	}
	return std::nullopt;
}

std::optional<Range<int>> parsePstateRangeLineWithRead(std::string title, AMDGPUData data) {
	auto contents = fileContents(data.devPath + "/pp_od_clk_voltage");
	if (!contents.has_value())
		return std::nullopt;

	return parsePstateRangeLine(title, *contents);
}

std::optional<std::pair<int, int>> parseLineValuePair(const std::string &line) {
	auto words = fplus::split_one_of(std::string{" "}, false, line);

	if (words.size() >= 3)
		return std::pair{std::stoi(words[1]), std::stoi(words[2])};
	return std::nullopt;
}

std::optional<int> parseLineValue(const std::string &line) {
	auto words = fplus::split_one_of(std::string{" "}, false, line);

	if (words.size() >= 2)
		return std::stoi(words[1]);
	return std::nullopt;
}

std::optional<VFPoint> vfPoint(const std::string &section, int index, const std::string &table) {
	auto lines = pstateSectionLines(section, table);
	if (lines.empty() && lines.size() < index + 1)
		return std::nullopt;

	auto line = lines[index];
	auto valuePair = parseLineValuePair(line);
	if (valuePair.has_value())
		return VFPoint{
		    .voltage = valuePair->second,
		    .clock = valuePair->first,
		};
	return std::nullopt;
}

// Same as above, but read the file
std::optional<VFPoint> vfPointWithRead(const std::string &section, int index, AMDGPUData data) {
	auto contents = fileContents(data.devPath + "/pp_od_clk_voltage");
	if (!contents.has_value())
		return std::nullopt;
	return vfPoint(section, index, *contents);
}

std::optional<PPTableType> fromPPTableContents(const std::string &contents) {
	auto clockSection = pstateSectionLines("OD_SCLK", contents);
	if (!clockSection.empty()) {
		// Vega 10 has the voltage-frequency curve labeled OD_SCLK
		if (parseLineValuePair(clockSection.front()).has_value())
			return Vega10;
		// On Vega 20 it's a section of single values
		if (parseLineValue(clockSection.front()).has_value()) {
			auto first = parsePstateRangeLine("VDDC_CURVE_VOLT[0]", contents);
			auto fourth = parsePstateRangeLine("VDDC_CURVE_VOLT[3]", contents);

			// Navi (NV1X?) has three frequency-voltage points
			if (first.has_value() && !fourth.has_value())
				return Navi;

			// RDNA 3 (SMU13) has six points using offsets
			if (first.has_value() && fourth.has_value())
				return SMU13;

			// Other tables of this type have min/max clock setting available
			return Vega20Other;
		}
	}
	return std::nullopt;
}

std::optional<AMDGPUData> fromRenderDFile(const fs::directory_entry &entry) {
	auto fd = open(entry.path().c_str(), O_RDONLY);
	auto v_ptr = drmGetVersion(fd);
	amdgpu_device_handle dev;
	uint32_t m, n;
	int devInitRetval = amdgpu_device_initialize(fd, &m, &n, &dev);
	// We can have multiple devices with the same PCI id, hopefully
	// this order is somewhat consistent
	static int gpuIndex = 0;
	if (fd > 0 && v_ptr && devInitRetval == 0 &&
	    std::string(v_ptr->name).find(_AMDGPU_NAME) != std::string::npos) {
		// Device uses amdgpu
		// Find hwmon path
		std::ostringstream stream;
		// Eg. renderD128
		auto filename = entry.path().filename().string();
		stream << "/sys/class/drm/" << filename << "/device/hwmon";

		auto devPath = "/sys/class/drm/" + filename + "/device";
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

		// Try to get powerplay table type
		std::optional<PPTableType> tableType = std::nullopt;
		auto contents = fileContents(devPath + "/pp_od_clk_voltage");
		if (contents.has_value())
			tableType = fromPPTableContents(*contents);

		auto index = gpuIndex;
		auto identifier = std::to_string(info.device_id) + std::to_string(index);
		gpuIndex++;
		drmFreeVersion(v_ptr);
		return AMDGPUData{
		    .hwmonPath = hwmonPath.value(),
		    .devPath = devPath,
		    .devHandle = dev,
		    .pciId = std::to_string(info.device_id),
		    .deviceFilename = filename,
		    .identifier = identifier,
		    .ppTableType = tableType,
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

int toMemoryClock(int controllerClock, AMDGPUData data) {
	drm_amdgpu_info_device info;
	if (amdgpu_query_info(data.devHandle, AMDGPU_INFO_DEV_INFO, sizeof(info), &info) != 0)
		return controllerClock;

	// For GDDR 6 (?) memory clock is 2x controller clock, for rest it's the same
	if (info.vram_type == AMDGPU_VRAM_TYPE_GDDR6)
		return controllerClock * 2;

	return controllerClock;
}

int toControllerClock(int memoryClock, AMDGPUData data) {
	drm_amdgpu_info_device info;
	if (amdgpu_query_info(data.devHandle, AMDGPU_INFO_DEV_INFO, sizeof(info), &info) != 0)
		return memoryClock;

	if (info.vram_type == AMDGPU_VRAM_TYPE_GDDR6)
		return memoryClock / 2;

	return memoryClock;
}
