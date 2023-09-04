#pragma once

class DeviceModel;
class QStackedWidget;
class QWidget;

// Data that (needs) to be accessed globally, eg. main stacked widget
namespace Globals {

// This is used to search nodes by their DBus path to avoid passing pointers through many levels
extern DeviceModel *g_deviceModel;
extern QStackedWidget *g_mainStack;
extern QWidget *g_deviceBrowser;

} // namespace Globals
