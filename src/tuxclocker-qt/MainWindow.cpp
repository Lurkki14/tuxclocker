#include <DBusTypes.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDebug>
#include <QString>
#include <QVector>

#include "MainWindow.hpp"

namespace TCDBus = TuxClocker::DBus;

Q_DECLARE_METATYPE(TCDBus::DeviceNode)
Q_DECLARE_METATYPE(TCDBus::FlatTreeNode<TCDBus::DeviceNode>)

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	qDBusRegisterMetaType<TCDBus::DeviceNode>();
	qDBusRegisterMetaType<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>();
	qDBusRegisterMetaType<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>>();
	
	auto conn = QDBusConnection::systemBus();
	QDBusInterface tuxclockerd("org.tuxclocker", "/", "org.tuxclocker", conn);
	
	QDBusReply<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>> reply = 
		tuxclockerd.call("flatDeviceTree");
		
	if (reply.isValid()) {
		auto flatTree = reply.value();
		for (auto &f_node : flatTree)
			qDebug() << f_node.childIndices << f_node.value.interface;
	}
}
