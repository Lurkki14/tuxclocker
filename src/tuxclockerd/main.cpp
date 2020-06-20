#include <DBusTypes.hpp>
#include <iostream>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMetaType>
#include <QDebug>
#include <patterns.hpp>
#include <Plugin.hpp>
#include <Tree.hpp>

#include "AdaptorFactory.hpp"

using namespace TuxClocker;
using namespace TuxClocker::Device;
using namespace TuxClocker::Plugin;
using namespace mpark::patterns;

namespace TCDBus = TuxClocker::DBus;

int main(int argc, char **argv) {
	QCoreApplication a(argc, argv);
	
	auto connection = QDBusConnection::systemBus();
	auto plugins = DevicePlugin::loadPlugins();
	QVector<QDBusAbstractAdaptor*> adaptors;
	QObject root;
	TreeNode<TCDBus::DeviceNode> dbusRootNode;
	
	std::function<void(TreeNode<DeviceNode>, QString, TreeNode<TCDBus::DeviceNode>*)> traverse;
	traverse = [&traverse, &connection, &adaptors, &root](
			TreeNode<DeviceNode> node,
			QString parentPath, TreeNode<TCDBus::DeviceNode> *dbusNode) {
		auto obj = new QObject(&root); // Is destroyed when root goes out of scope
		auto objName = QString::fromStdString(node.value().hash).replace(" ", "");
		auto thisPath = parentPath + objName;
		QString ifaceName;
		adaptors.append(new NodeAdaptor(obj, node.value()));
		if_let(pattern(some(arg)) = node.value().interface) = [&](auto iface) {
			if_let(pattern(some(arg)) = 
					AdaptorFactory::adaptor(obj, iface)) = [&](auto adaptor) {
				adaptors.append(adaptor);
				// TODO: don't hardcode interface name
				// Maybe create an intermediate class that contains the interface name to avoid this
				// For some reason this throws a bad_cast when using 'as' with pointer type
				match(*adaptor)
					(pattern(as<AssignableAdaptor>(_)) = [&] {
						ifaceName = "org.tuxclocker.Assignable";
					},
					pattern(as<DynamicReadableAdaptor>(_)) = [&] {
						ifaceName = "org.tuxclocker.DynamicReadable";
					},
					pattern(as<StaticReadableAdaptor>(_)) = [&] {
						ifaceName = "org.tuxclocker.StaticReadable";
					},
					pattern(_) = [] {});
			};
		};
		if (!connection.registerObject(thisPath, obj))
			qDebug() << "Couldn't register object at path" << thisPath << connection.lastError();
		
		auto thisDBusNode = TreeNode<TCDBus::DeviceNode>{{ifaceName, thisPath}};
		dbusNode->appendChild(thisDBusNode);
		qDebug() << thisPath;
		for (const auto &child : node.children())
			traverse(child, thisPath + "/", &dbusNode->childrenPtr()->back());
	};
	
	TreeNode<DeviceNode> lvl1nodes;
	if (plugins.has_value()) {
		qDebug() << "found " << plugins.value().size() << " plugins";
		for (auto &plugin : plugins.value()) {
			//rootNodes.append(plugin->deviceRootNode());
			// Root node should always be empty
			for (const auto &node : plugin->deviceRootNode().children()) {
				lvl1nodes.appendChild(node);
				traverse(node, "/", &dbusRootNode);
				
				TreeNode<DeviceNode>::preorder(node, [](auto val) {
					qDebug() << QString::fromStdString(val.name);
				});
			}
		}
	}
	auto ma = new MainAdaptor(&root, dbusRootNode);
	connection.registerObject("/", &root);
	
	if (!connection.registerService("org.tuxclocker")) {
		qDebug() << "unable to register:" << connection.lastError().message();
	}
	
	return a.exec();
}
