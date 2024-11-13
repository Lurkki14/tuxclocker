#include "DeviceBrowser.hpp"
#include "AssignableItemData.hpp"
#include "./monitor/Monitor.hpp"
#include "./monitor/Metric.hpp"
#include "qnamespace.h"

#include <Globals.hpp>
#include <libintl.h>
#include <MainWindow.hpp>
#include <patterns.hpp>
#include <QStackedWidget>
#include <QToolButton>
#include <QVariant>
#include <Settings.hpp>
#include <Utils.hpp>
#include <QMimeData>
#include <QStandardItemModel>

#define _(String) gettext(String)

using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(AssignableProxy *)

class MonitorDropBox : public QLabel{
public:
	Monitor monitor;
	QWidget widget;

	MonitorDropBox(QWidget *parent) : QLabel(parent), monitor(nullptr), widget(nullptr){
		setAcceptDrops(true);
		setFrameStyle(QFrame::Box);
		setLineWidth(1);
		setText("Monitor");
		setAlignment(Qt::AlignCenter);
	}

	void setupMonitor(QStringList dbus_paths){
		monitor.setParent(&widget);
		widget.show();

		monitor.metrics.clear();

		for(QString dbus_path : dbus_paths){
		  monitor.metrics.append(std::make_shared<Metric>(dbus_path));
		}

		monitor.setupGraphs();

		monitor.start();
	}

	void dropEvent(QDropEvent *event) override {
		QStringList dbus_paths = event->mimeData()->text().split(",");

		if(dbus_paths.count()){
			setupMonitor(dbus_paths);
		}
	}

	void dragEnterEvent(QDragEnterEvent *event) override {
		event->acceptProposedAction();
	}

};

DeviceBrowser::DeviceBrowser(DeviceModel &model, QWidget *parent)
    : QWidget(parent), m_deviceModel(model) {
	m_layout = new QGridLayout(this);
	m_proxyModel = new DeviceProxyModel(model, this);
	m_treeView = new DeviceTreeView;
	m_treeView->setModel(m_proxyModel);

	m_flagLabel = new QLabel(_("Showing:"));
	m_apply = new QPushButton(_("Apply changes"));
	m_apply->setEnabled(true);

	m_flagEditor = new FlagEditor(
	    QVector({std::tuple(QString(_("Assignables")), DeviceModel::assignableIcon(),
			 DeviceModel::Assignable),
		std::tuple(QString(_("Dynamic Values")), DeviceModel::dynamicReadableIcon(),
		    DeviceModel::DynamicReadable),
		std::tuple(QString(_("Static Values")), DeviceModel::staticReadableIcon(),
		    DeviceModel::StaticReadable)}),
	    this);

	connect(m_apply, &QPushButton::pressed, &m_deviceModel, &DeviceModel::applyChanges);

	m_flagEditor->setFlags(DeviceModel::AllInterfaces);

	m_flagEditor->flagsChanged.connect([=](auto flags) { m_proxyModel->setFlags(flags); });

	m_settings = nullptr;

	auto icon = QIcon{":/settings.svg"};
	auto toolButton = new QToolButton{this};
	toolButton->setIcon(icon);

	connect(toolButton, &QToolButton::released, [=] {
		if (!m_settings) {
			m_settings = new Settings{this};
			Globals::g_mainStack->addWidget(m_settings);

			connect(m_settings, &Settings::cancelled,
			    [=] { Globals::g_mainStack->setCurrentWidget(this); });

			connect(m_settings, &Settings::settingsSaved, [=](auto data) {
				Globals::g_mainStack->setCurrentWidget(this);
				// No-op when data.assignableSettings is empty
				Utils::setModelAssignableSettings(
				    *Globals::g_deviceModel, data.assignableSettings);

				if (data.autoApplyProfile && data.currentProfile.has_value())
					m_deviceModel.applyChanges();

				Globals::g_mainWindow->setTrayIconEnabled(data.useTrayIcon);
			});
		}
		Globals::g_mainStack->setCurrentWidget(m_settings);
	});

	auto monitorDropBox = new MonitorDropBox{this};

	m_layout->addWidget(toolButton, 0, 0);
	m_layout->addWidget(monitorDropBox, 0, 1);

	m_layout->addWidget(m_flagLabel, 0, 2, 1, 0, Qt::AlignRight);
	m_layout->addWidget(m_flagEditor, 0, 2);
	m_layout->addWidget(m_treeView, 1, 0, 1, 3);
	m_layout->addWidget(m_apply, 2, 0, 1, 3);

	setLayout(m_layout);
}
