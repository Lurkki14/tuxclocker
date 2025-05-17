#include <Plugin.hpp>

#include <filesystem>
#include <sstream>

using namespace TuxClocker::Plugin;

namespace fs = std::filesystem;

std::string Plugin::pluginPath() { return TC_PLUGIN_PATH; }

std::optional<std::vector<std::shared_ptr<DevicePlugin>>> DevicePlugin::loadPlugins() {
	std::vector<std::shared_ptr<DevicePlugin>> retval;

	std::string pluginPath;
	const char *pluginPathEnv = std::getenv("TUXCLOCKER_PLUGIN_PATH");

	// Use plugin path from environment if it exists
	if (pluginPathEnv)
		pluginPath = pluginPathEnv;
	else
		pluginPath = Plugin::pluginPath();

	for (const fs::directory_entry &entry : fs::directory_iterator(pluginPath)) {
		// Bleh, have to catch this unless I do more manual checks
		try {
			auto plugin = boost::dll::import_symbol<DevicePlugin>(
			    entry.path().string(), TUXCLOCKER_PLUGIN_SYMBOL_NAME);
			retval.push_back(plugin);
			std::cout << "found plugin at " << entry.path().string() << "\n";
		} catch (boost::system::system_error &e) {
		}
	}

	if (retval.empty())
		return std::nullopt;
	return retval;
}
