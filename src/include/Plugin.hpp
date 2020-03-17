#pragma once

#include <boost/config.hpp>
#include <boost/dll/import.hpp>
#include <optional>
#include <string>

#include "Device.hpp"
#include "Tree.hpp"

#define TUXCLOCKER_PLUGIN_EXPORT(PluginType) \
	extern "C" BOOST_SYMBOL_EXPORT PluginType __plugin; \
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
	static std::string pluginDirName() {return "plugins";}
	// Full path is efined at compile time
	static std::string pluginPath();
};

class DevicePlugin {
public:
	// Communicate plugin initialization success in this way since constructors cannot communicate it.
	virtual std::optional<InitializationError> initializationError() = 0;
	virtual void foo() = 0;
	virtual TreeNode<DeviceNode> deviceRootNode() = 0;
	virtual ~DevicePlugin() {}
	
	// Helper for loading all DevicePlugin's
	static std::optional<std::vector<boost::shared_ptr<DevicePlugin>>> loadPlugins();
};

};
};
