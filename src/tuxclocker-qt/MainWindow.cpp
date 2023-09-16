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
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QString>
#include <QTreeView>
#include <QVector>
#include <DeviceModelDelegate.hpp>
#include <Globals.hpp>
#include <Tree.hpp>
#include <Utils.hpp>

namespace TCDBus = TuxClocker::DBus;

using namespace TuxClocker;

Q_DECLARE_METATYPE(TCDBus::DeviceNode)
Q_DECLARE_METATYPE(TCDBus::FlatTreeNode<TCDBus::DeviceNode>)

DeviceModel *Globals::g_deviceModel;
QStackedWidget *Globals::g_mainStack;
QWidget *Globals::g_deviceBrowser;
SettingsData Globals::g_settingsData;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	qDBusRegisterMetaType<TCDBus::DeviceNode>();
	qDBusRegisterMetaType<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>();
	qDBusRegisterMetaType<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>>();

	restoreGeometryFromCache(this);

	setWindowIcon(QIcon{":/tuxclocker-logo.svg"});

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

	auto model = new DeviceModel(root);
	auto browser = new DeviceBrowser(*model);

	auto stack = new QStackedWidget(this);
	stack->addWidget(browser);
	setCentralWidget(stack);

	// TODO: make sure this is the only assignment
	Globals::g_deviceBrowser = browser;
	Globals::g_deviceModel = model;
	Globals::g_mainStack = stack;
	Globals::g_settingsData = Settings::readSettings();

	Utils::setModelAssignableSettings(*model, Globals::g_settingsData.assignableSettings);

	if (Globals::g_settingsData.autoApplyProfile)
		model->applyChanges();

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
