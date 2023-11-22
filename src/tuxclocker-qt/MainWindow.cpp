#include "MainWindow.hpp"

#include <DBusTypes.hpp>
#include <DeviceBrowser.hpp>
#include <DeviceModel.hpp>
#include <DeviceModelDelegate.hpp>
#include <libintl.h>
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
#include <QSystemTrayIcon>
#include <QTreeView>
#include <QVector>
#include <DeviceModelDelegate.hpp>
#include <Globals.hpp>
#include <Tree.hpp>
#include <Utils.hpp>

#define _(String) gettext(String)

namespace TCDBus = TuxClocker::DBus;

using namespace TuxClocker;

Q_DECLARE_METATYPE(TCDBus::DeviceNode)
Q_DECLARE_METATYPE(TCDBus::FlatTreeNode<TCDBus::DeviceNode>)

DeviceModel *Globals::g_deviceModel;
MainWindow *Globals::g_mainWindow;
QStackedWidget *Globals::g_mainStack;
QWidget *Globals::g_deviceBrowser;
SettingsData Globals::g_settingsData;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
	qDBusRegisterMetaType<TCDBus::DeviceNode>();
	qDBusRegisterMetaType<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>();
	qDBusRegisterMetaType<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>>();

	restoreGeometryFromCache(this);

	setWindowIcon(QIcon{":/tuxclocker-logo.svg"});
	setWindowTitle("TuxClocker");

	auto conn = QDBusConnection::systemBus();
	QDBusInterface tuxclockerd("org.tuxclocker", "/", "org.tuxclocker", conn);

	if (tuxclockerd.lastError().isValid())
		qDebug() << "Couldn't connect to TuxClocker daemon:"
			 << tuxclockerd.lastError().message();

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
	Globals::g_mainWindow = this;

	Utils::setModelAssignableSettings(*model, Globals::g_settingsData.assignableSettings);

	if (Globals::g_settingsData.autoApplyProfile)
		model->applyChanges();

	Utils::writeAssignableDefaults(*model);

	// Enable tray icon when enabled in settings
	m_trayIcon = nullptr;
	setTrayIconEnabled(Globals::g_settingsData.useTrayIcon);
}

void MainWindow::setTrayIconEnabled(bool enable) {
	if (enable) {
		if (!m_trayIcon)
			// This seems to make the main window not close during closeEvent
			m_trayIcon = new QSystemTrayIcon{this};
		m_trayIcon->setIcon(QIcon{QPixmap{":/tuxclocker-logo.svg"}});
		m_trayIcon->setToolTip("TuxClocker");
		m_trayIcon->setContextMenu(createTrayMenu());
		m_trayIcon->show();
		return;
	}
	// Remove tray icon
	if (m_trayIcon)
		delete m_trayIcon;
}

QMenu *MainWindow::createTrayMenu() {
	auto menu = new QMenu{this};

	auto show = new QAction{_("&Maximize TuxClocker"), this};
	connect(show, &QAction::triggered, this, &MainWindow::show);
	menu->addAction(show);

	auto quit = new QAction{_("&Quit"), this};
	connect(quit, &QAction::triggered, this, &QApplication::quit);
	menu->addAction(quit);

	return menu;
}

void MainWindow::restoreGeometryFromCache(QWidget *widget) {
	auto cacheFilePath = Utils::cacheFilePath();

	QSettings settings{cacheFilePath, QSettings::NativeFormat};
	widget->restoreGeometry(settings.value("geometry").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event) {
	// Save window geometry to user cache dir (XDG_CACHE_HOME on Linux)
	auto cacheFilePath = Utils::cacheFilePath();

	QSettings settings{cacheFilePath, QSettings::NativeFormat};
	settings.setValue("geometry", saveGeometry());
	QWidget::closeEvent(event);
}
