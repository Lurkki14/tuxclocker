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
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
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

	if (tuxclockerd.lastError().isValid()) {
		QMessageBox{QMessageBox::Critical, "TuxClocker",
		    QString{_("Couldn't connect to TuxClocker daemon: %1")}.arg(
			tuxclockerd.lastError().message())}
		    .exec();
	}

	QDBusReply<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>> reply =
	    tuxclockerd.call("flatDeviceTree");

	// Convert to non-dbus flat tree
	FlatTree<TCDBus::DeviceNode> flatTree;
	if (reply.isValid()) {
		auto dbusFlatTree = reply.value();
		for (auto &f_node : dbusFlatTree) {
			// qDebug() << f_node.value.interface << f_node.value.path;
			FlatTreeNode<TCDBus::DeviceNode> node{
				f_node.value, std::vector<int>(f_node.childIndices.begin(), f_node.childIndices.end())};
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

	if (m_startMinimized) {
		QTimer::singleShot(0, this, [this]() {
			if (m_trayIcon)
				hide();
			else
				showMinimized();
		});
	}
}

void MainWindow::setTrayIconEnabled(bool enable) {
	if (enable) {
		if (!m_trayIcon) {
			// This seems to make the main window not close during closeEvent
			m_trayIcon = new QSystemTrayIcon{this};
			m_trayIcon->setIcon(QIcon{QPixmap{":/tuxclocker-logo.svg"}});
			m_trayIcon->setToolTip("TuxClocker");
			m_trayIcon->setContextMenu(createTrayMenu());
			m_trayIcon->show();
			connect(m_trayIcon, &QSystemTrayIcon::activated, this,
			    [this](QSystemTrayIcon::ActivationReason reason) {
				if (reason == QSystemTrayIcon::Trigger ||
				    reason == QSystemTrayIcon::DoubleClick)
					restoreWindow();
			    });
		}
		return;
	}
	// Remove tray icon
	if (m_trayIcon) {
		delete m_trayIcon;
		m_trayIcon = nullptr;
	}
}

QMenu *MainWindow::createTrayMenu() {
	auto menu = new QMenu{this};

	auto show = new QAction{_("&Show TuxClocker"), this};
	connect(show, &QAction::triggered, this, &MainWindow::restoreWindow);
	menu->addAction(show);

	auto hide = new QAction{_("&Hide TuxClocker"), this};
	connect(hide, &QAction::triggered, this, &MainWindow::hide);
	menu->addAction(hide);

	auto quit = new QAction{_("&Quit"), this};
	connect(quit, &QAction::triggered, this, [this]() {
		saveStateToCache();
		QApplication::quit();
	});
	menu->addAction(quit);

	return menu;
}

void MainWindow::restoreGeometryFromCache(QWidget *widget) {
	auto cacheFilePath = Utils::cacheFilePath();

	QSettings settings{cacheFilePath, QSettings::NativeFormat};
	widget->restoreGeometry(settings.value("geometry").toByteArray());
	m_startMinimized = settings.value("minimized", false).toBool();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	// if the tray icon is active, hide the application instead of closing it
	if (m_trayIcon && m_trayIcon->isVisible()) {
		auto cacheFilePath = Utils::cacheFilePath();
		QSettings settings{cacheFilePath, QSettings::NativeFormat};
		if (!settings.value("trayCloseNoticeShown", false).toBool()) {
			QMessageBox::information(this, tr("TuxClocker"),
			    tr("TuxClocker will continue to run "
			       "in the background. To completely "
			       "exit the application, choose <b><u>Q</u>uit</b> "
			       "from the system tray icon"));
			settings.setValue("trayCloseNoticeShown", true);
		}
		hide();
		event->ignore();
		return;
	}

	saveStateToCache();
	QWidget::closeEvent(event);
}

void MainWindow::restoreWindow() {
	setWindowState((windowState() & ~Qt::WindowMinimized));
	showNormal();
}

void MainWindow::saveStateToCache() {
	// Save window geometry to user cache dir (XDG_CACHE_HOME on Linux)
	auto cacheFilePath = Utils::cacheFilePath();

	QSettings settings{cacheFilePath, QSettings::NativeFormat};
	settings.setValue("geometry", saveGeometry());
	settings.setValue("minimized", isMinimized() || isHidden());
}
