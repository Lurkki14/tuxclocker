#include <Plugin.hpp>

#include <filesystem>
#include <sstream>

using namespace TuxClocker::Plugin;

namespace fs = std::filesystem;

std::string Plugin::pluginPath() {
	return TC_PLUGIN_PATH;
}

std::optional<std::vector<boost::shared_ptr<DevicePlugin>>> DevicePlugin::loadPlugins() {
	std::vector<boost::shared_ptr<DevicePlugin>> retval;
	
	for (const fs::directory_entry &entry : fs::directory_iterator(Plugin::pluginPath())) {
		// Bleh, have to catch this unless I do more manual checks
		try {
			auto plugin = dll::import<DevicePlugin>(entry.path().string(), TUXCLOCKER_PLUGIN_SYMBOL_NAME);
			retval.push_back(plugin);
		}
		catch (boost::system::system_error &e) {}
	}
	
	if (retval.empty()) return std::nullopt;
	return retval;
}