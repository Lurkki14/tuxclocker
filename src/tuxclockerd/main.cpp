#include <DBusTypes.hpp>
#include <iostream>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMetaType>
#include <QDebug>
#include <QStack>
#include <patterns.hpp>
#include <Plugin.hpp>
#include <Tree.hpp>

#include "AdaptorFactory.hpp"

using namespace TuxClocker;
using namespace TuxClocker::Device;
using namespace TuxClocker::Plugin;
using namespace mpark::patterns;

int main(int argc, char **argv) {
	QCoreApplication a(argc, argv);
	
	auto connection = QDBusConnection::systemBus();
	auto plugins = DevicePlugin::loadPlugins();
	QVector<QDBusAbstractAdaptor*> adaptors;
	
	QObject root;
	if (plugins.has_value()) {
		qDebug() << "found " << plugins.value().size() << " plugins";
		for (auto &plugin : plugins.value()) {
			auto n = plugin->deviceRootNode();
			TreeNode<DeviceNode>::preorder(plugin->deviceRootNode(), [](auto val) {qDebug() << QString::fromStdString(val.name);});
			

			/*TreeNode<DeviceNode>::preorder(plugin->deviceRootNode(), [&](auto nodeVal) {
				auto obj = new QObject(&root); // Is destroyed when root goes out of scope
				// Remove whitespaces to make valid object paths
				auto objName = QString::fromStdString(nodeVal.name).replace(" ", "");
				
				if_let(pattern(some(arg)) = nodeVal.interface) = [&](auto iface) {
					adaptors.append(AdaptorFactory::adaptor(obj, iface).value());
				};
				connection.registerObject("/" + objName, obj);
			});*/
			
			std::function<void(TreeNode<DeviceNode>, QString)> traverse;
			traverse = [&traverse, &connection, &adaptors, &root](
					TreeNode<DeviceNode> node,
					QString parentPath) {
				auto obj = new QObject(&root); // Is destroyed when root goes out of scope
				// Remove whitespaces to make valid object paths
				auto objName = QString::fromStdString(node.value().name).replace(" ", "");
				
				if_let(pattern(some(arg)) = node.value().interface) = [&](auto iface) {
					adaptors.append(AdaptorFactory::adaptor(obj, iface).value());
				};
				auto thisPath = parentPath + objName + "/";
				connection.registerObject("/" + objName, obj);	
				qDebug() << thisPath;
				for (const auto &child : node.children()) traverse(child, thisPath);
			};
			// Root node should always be empty
			for (const auto &node : plugin->deviceRootNode().children())
				traverse(node, "/");
		}
	}
	
	//registerReadables(&root, connection);
	//registerAssignables(&root, connection);
	
	//DummyAdaptor ad(&root);
	connection.registerObject("/", &root);
	
	if (!connection.registerService("org.tuxclocker")) {
		qDebug() << "unable to register:" << connection.lastError().message();
	}
	
	return a.exec();
}
