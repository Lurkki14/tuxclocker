#include "HWData.hpp"

#include <fplus/fplus.hpp>
#include <Utils.hpp>

std::optional<PciData> fromUeventFile(const std::string &deviceFilename) {
	/*
	DRIVER=nvidia
	PCI_CLASS=30000
	PCI_ID=10DE:1B80
	PCI_SUBSYS_ID=1458:3702
	PCI_SLOT_NAME=0000:01:00.0
	MODALIAS=pci:v000010DEd00001B80sv00001458sd00003702bc03sc00i00
	*/

	char path[64];
	snprintf(path, 64, "/sys/class/drm/%s/device/uevent", deviceFilename.c_str());

	auto contents = fileContents(path);
	if (!contents.has_value())
		return std::nullopt;

	auto isNewline = [](char c) { return c == '\n'; };
	auto lines = fplus::split_by(isNewline, false, *contents);

	if (lines.size() > 5) {
		auto idWords = fplus::split_one_of(std::string{"=:"}, false, lines[2]);
		auto subsysWords = fplus::split_one_of(std::string{"="}, false, lines[3]);
		if (idWords.size() > 2 && subsysWords.size() > 1) {
			auto device = fplus::to_lower_case(idWords[2]);
			auto subsystem = fplus::to_lower_case(subsysWords[1]);

			return PciData{device, subsystem};
		}
	}
	return std::nullopt;
}

std::optional<PyObject *> getPciObject() {
	auto modName = PyUnicode_FromString("hwdata");
	auto mod = PyImport_Import(modName);

	if (mod) {
		auto pciObj = PyObject_GetAttrString(mod, "PCI");
		if (pciObj)
			return pciObj;
	}
	return std::nullopt;
}

std::optional<std::string> hwdataName(PyObject *pciObj, PciData data) {
	// Vendor is always '1002' aka AMD
	auto subsysStr = PyObject_CallMethod(
	    pciObj, "get_subsystem", "sss", "1002", data.device.c_str(), data.subsystem.c_str());
	if (subsysStr && PyUnicode_Check(subsysStr))
		return "AMD " + std::string{PyUnicode_AsUTF8(subsysStr)};

	// Try to get device name
	auto devStr = PyObject_CallMethod(pciObj, "get_device", "ss", "1002", data.device.c_str());
	if (devStr && PyUnicode_Check(devStr))
		return "AMD " + std::string{PyUnicode_AsUTF8(devStr)};
	return std::nullopt;
}
