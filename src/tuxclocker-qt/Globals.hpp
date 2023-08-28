#pragma once

class DeviceModel;

// Data that (needs) to be accessed globally, eg. main stacked widget
namespace Globals {

// This is used to search nodes by their DBus path to avoid passing pointers through many levels
extern DeviceModel *g_deviceModel;

} // namespace Globals
