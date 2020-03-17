#include <Plugin.hpp>

#include <iostream>

using namespace TuxClocker::Plugin;
using namespace TuxClocker::Device;
using namespace TuxClocker;

class AMDPlugin : public DevicePlugin {
public:
	AMDPlugin();
	std::optional<InitializationError> initializationError() {return std::nullopt;}
	void foo() {std::cout << "hi from AMD plugin\n";}
	TreeNode<DeviceNode> deviceRootNode();
	~AMDPlugin() {}
private:
	TreeNode<DeviceNode> m_rootNode;
};

AMDPlugin::AMDPlugin() {
	/*Assignable a([](auto arg) {
		return std::nullopt;
	});
	
	DeviceNode root{"AMD", a};
	m_rootNode.appendChild(root);*/
}

TreeNode<DeviceNode> AMDPlugin::deviceRootNode() {
;
	return m_rootNode;
}

TUXCLOCKER_PLUGIN_EXPORT(AMDPlugin)
