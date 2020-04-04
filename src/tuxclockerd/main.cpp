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
//#include "DummyAdaptor.hpp"

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
			TreeNode<DeviceNode>::preorder(plugin->deviceRootNode(), [](auto val) {std::cout << val.name << "\n";});
			
			TreeNode<DeviceNode>::preorder(plugin->deviceRootNode(), [&](auto nodeVal) {
				auto obj = new QObject(&root); // Is destroyed when root goes out of scope
				auto objName = QString::fromStdString(nodeVal.name).replace(" ", "");
				
				if_let(pattern(some(arg)) = nodeVal.interface) = [&](auto iface) {
					adaptors.append(AdaptorFactory::adaptor(obj, iface).value());
				};
				
				connection.registerObject("/" + objName, obj);
			});
			
		}
	}
	
	//registerReadables(&root, connection);
	//registerAssignables(&root, connection);
	
	DummyAdaptor ad(&root);
	connection.registerObject("/", &root);
	
	if (!connection.registerService("org.tuxclocker")) {
		qDebug() << "unable to register:" << connection.lastError().message();
	}
	
	return a.exec();
}
