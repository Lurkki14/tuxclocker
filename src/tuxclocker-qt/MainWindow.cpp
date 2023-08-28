#include "MainWindow.hpp"

#include <DBusTypes.hpp>
#include <DeviceBrowser.hpp>
#include <DeviceModel.hpp>
#include <DeviceModelDelegate.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDebug>
#include <QSettings>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QString>
#include <QTreeView>
#include <QVector>
#include <Globals.hpp>
#include <Tree.hpp>
#include <Utils.hpp>

namespace TCDBus = TuxClocker::DBus;

using namespace TuxClocker;

Q_DECLARE_METATYPE(TCDBus::DeviceNode)
Q_DECLARE_METATYPE(TCDBus::FlatTreeNode<TCDBus::DeviceNode>)

DeviceModel *Globals::g_deviceModel;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	qDBusRegisterMetaType<TCDBus::DeviceNode>();
	qDBusRegisterMetaType<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>();
	qDBusRegisterMetaType<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>>();

	restoreGeometryFromCache(this);

	auto conn = QDBusConnection::systemBus();
	QDBusInterface tuxclockerd("org.tuxclocker", "/", "org.tuxclocker", conn);

	QDBusReply<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>> reply =
	    tuxclockerd.call("flatDeviceTree");

	// Convert to non-dbus flat tree
	FlatTree<TCDBus::DeviceNode> flatTree;
	if (reply.isValid()) {
		auto dbusFlatTree = reply.value();
		for (auto &f_node : dbusFlatTree) {
			// qDebug() << f_node.value.interface << f_node.value.path;
			FlatTreeNode<TCDBus::DeviceNode> node{
			    f_node.value, f_node.childIndices.toStdVector()};
			flatTree.nodes.push_back(node);
		}
	}

	auto root = flatTree.toTree(flatTree);

	/*auto view = new QTreeView;
	view->setItemDelegate(new DeviceModelDelegate);
	view->setModel(model);
	setCentralWidget(view);*/

	auto model = new DeviceModel(root);
	auto browser = new DeviceBrowser(*model);
	setCentralWidget(browser);

	// TODO: make sure this is the only assignment
	Globals::g_deviceModel = model;

	Utils::writeAssignableDefaults(*model);
}

void MainWindow::restoreGeometryFromCache(QWidget *widget) {
	auto cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	auto cacheFilePath = QString("%1/tuxclocker.conf").arg(cacheDir);

	QSettings settings{cacheFilePath, QSettings::NativeFormat};
	widget->restoreGeometry(settings.value("geometry").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event) {
	// Save window geometry to user cache dir (XDG_CACHE_HOME on Linux)
	auto cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	auto cacheFilePath = QString("%1/tuxclocker.conf").arg(cacheDir);

	QSettings settings{cacheFilePath, QSettings::NativeFormat};
	settings.setValue("geometry", saveGeometry());
	QWidget::closeEvent(event);
}
