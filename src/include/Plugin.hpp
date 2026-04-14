#pragma once

#include <boost/config.hpp>
#include <boost/dll/import.hpp>
#include <boost/version.hpp>
#include <optional>
#include <string>

// boost::dll::import_symbol returns std::shared_ptr starting from Boost 1.88
#if BOOST_VERSION >= 108800
	#include <memory>
	#define TC_PLUGIN_PTR std::shared_ptr
#else
	#include <boost/shared_ptr.hpp>
	#define TC_PLUGIN_PTR boost::shared_ptr
#endif

#include "Device.hpp"
#include "Tree.hpp"

#define TUXCLOCKER_PLUGIN_EXPORT(PluginType)                                                       \
	extern "C" BOOST_SYMBOL_EXPORT PluginType __plugin;                                        \
	PluginType __plugin;

#define TUXCLOCKER_PLUGIN_SYMBOL_NAME "__plugin"

namespace TuxClocker {
namespace Plugin {

namespace dll = boost::dll;

enum class InitializationError {
	UnknownError
};

using namespace TuxClocker::Device;

class Plugin {
public:
	static std::string pluginDirName() { return "plugins"; }
	// Full path is efined at compile time
	static std::string pluginPath();
};

class DevicePlugin {
public:
	// Communicate plugin initialization success in this way since constructors cannot
	// communicate it.
	virtual std::optional<InitializationError> initializationError() = 0;
	virtual TreeNode<DeviceNode> deviceRootNode() = 0;
	virtual ~DevicePlugin() {}

	// Helper for loading all DevicePlugin's
	static std::optional<std::vector<TC_PLUGIN_PTR<DevicePlugin>>> loadPlugins();
};

}; // namespace Plugin
}; // namespace TuxClocker
