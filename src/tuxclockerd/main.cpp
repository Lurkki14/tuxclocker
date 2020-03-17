#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMetaType>
#include <QDebug>
#include <Plugin.hpp>

#include "AssignableAdaptorFactory.h"
#include "EnumData.h"
#include "Result.h"
#include "ReadableAdaptorFactory.h"

using namespace TuxClocker;

int main(int argc, char **argv) {
	QCoreApplication a(argc, argv);
	
	auto plugins = Plugin::DevicePlugin::loadPlugins();
	
	if (plugins.has_value()) {
		qDebug() << "found " << plugins.value().size() << " plugins";
		for (auto &plugin : plugins.value()) {
			plugin->foo();
			plugin->deviceRootNode();
		}
	}
	/*registerMetatypes();
	
	QObject root;
	
	auto connection = QDBusConnection::systemBus();
	registerReadables(&root, connection);
	registerAssignables(&root, connection);
	
	connection.registerObject("/", &root);
	
	if (!connection.registerService("org.tuxclocker")) {
		qDebug() << "unable to register:" << connection.lastError().message();
	}*/
	
	return a.exec();
}
