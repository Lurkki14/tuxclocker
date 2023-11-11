#include <optional>
#include <Python.h>
#include <string>

// Reduces the places we need to ifdef by destroying automatically
class PythonInstance {
public:
	PythonInstance() { Py_Initialize(); }
	~PythonInstance() { Py_Finalize(); }
};

struct PciData {
	std::string device;
	std::string subsystem;
};

std::optional<PciData> fromUeventFile(const std::string &deviceFilename);
std::optional<PyObject *> getPciObject();
std::optional<std::string> hwdataName(PyObject *pciObj, PciData);
