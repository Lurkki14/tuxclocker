#include <DBusTypes.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDebug>
#include <QStandardItemModel>
#include <QString>
#include <QTreeView>
#include <QVector>
#include <Tree.hpp>

#include <DeviceModel.hpp>
#include <DeviceModelDelegate.hpp>
#include "MainWindow.hpp"

namespace TCDBus = TuxClocker::DBus;

using namespace TuxClocker;

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
	
	// Convert to non-dbus flat tree
	FlatTree<TCDBus::DeviceNode> flatTree;
	if (reply.isValid()) {
		auto dbusFlatTree = reply.value();
		for (auto &f_node : dbusFlatTree) {
			//qDebug() << f_node.value.interface << f_node.value.path;
			FlatTreeNode<TCDBus::DeviceNode> node{
				f_node.value,
				f_node.childIndices.toStdVector()
			};
			flatTree.nodes.push_back(node);
		}
	}
	
	auto root = flatTree.toTree(flatTree);
	
	auto view = new QTreeView;
	view->setItemDelegate(new DeviceModelDelegate);
	auto model = new DeviceModel(root);
	view->setModel(model);
	setCentralWidget(view);
}
