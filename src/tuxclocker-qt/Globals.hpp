#pragma once

#include <Settings.hpp>

class DeviceModel;
class MainWindow;
class QStackedWidget;
class QWidget;

// Data that (needs) to be accessed globally, eg. main stacked widget
namespace Globals {

// This is used to search nodes by their DBus path to avoid passing pointers through many levels
extern DeviceModel *g_deviceModel;
extern MainWindow *g_mainWindow;
extern QStackedWidget *g_mainStack;
extern QWidget *g_deviceBrowser;
// When applying successfully, we need to know which profile to save changes to
extern SettingsData g_settingsData;

} // namespace Globals
