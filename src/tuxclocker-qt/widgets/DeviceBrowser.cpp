#include "DeviceBrowser.hpp"
#include "AssignableItemData.hpp"
#include "qnamespace.h"

#include <patterns.hpp>
#include <QVariant>

using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(AssignableProxy *)

DeviceBrowser::DeviceBrowser(DeviceModel &model, QWidget *parent)
    : QWidget(parent), m_deviceModel(model) {
	m_layout = new QGridLayout(this);
	m_proxyModel = new DeviceProxyModel(model, this);
	m_treeView = new DeviceTreeView;
	m_treeView->setModel(m_proxyModel);

	// Expand all nodes
	// TODO: remember collapsed nodes
	m_treeView->expandAll();

	m_flagLabel = new QLabel("Showing:");
	m_apply = new QPushButton("Apply changes");
	m_apply->setEnabled(true);

	m_flagEditor = new FlagEditor(
	    QVector({std::tuple(QString("Assignables"), DeviceModel::assignableIcon(),
			 DeviceModel::Assignable),
		std::tuple(QString("Dynamic Values"), DeviceModel::dynamicReadableIcon(),
		    DeviceModel::DynamicReadable),
		std::tuple(QString("Static Values"), DeviceModel::staticReadableIcon(),
		    DeviceModel::StaticReadable)}),
	    this);

	connect(m_apply, &QPushButton::pressed, &m_deviceModel, &DeviceModel::applyChanges);

	m_treeView->functionEditorRequested.connect([this](QModelIndex &index) {
		auto a_data = index.data(DeviceModel::AssignableRole);
		if (!a_data.isValid())
			return;

		auto a_info = a_data.value<AssignableItemData>();
		auto proxy =
		    index.data(DeviceModel::AssignableProxyRole).value<AssignableProxy *>();
		auto name = index.data(DeviceModel::NodeNameRole).toString();
		match(a_info.assignableInfo())(
		    pattern(as<RangeInfo>(arg)) =
			[=](auto ri) {
				auto f_editor = new FunctionEditor{m_deviceModel, ri, *proxy, name};
				f_editor->show();
				f_editor->assignableConnectionChanged.connect(
				    [=](auto conn) { proxy->startConnection(conn); });
			},
		    pattern(_) = [] {});

		/*auto f_editor = new FunctionEditor(m_deviceModel, rangeInfo, proxy);
		f_editor->show();

		f_editor->assignableConnectionChanged.connect(
				[&proxy] (auto assignableConnection) {
			proxy.startConnection(assignableConnection);
		});*/
	});

	m_flagEditor->setFlags(DeviceModel::AllInterfaces);

	m_flagEditor->flagsChanged.connect([=](auto flags) { m_proxyModel->setFlags(flags); });

	m_layout->addWidget(m_flagLabel, 0, 0);
	m_layout->addWidget(m_flagEditor, 0, 1);
	m_layout->addWidget(m_treeView, 1, 0, 1, 2);
	m_layout->addWidget(m_apply, 2, 0, 1, 2);

	setLayout(m_layout);
}
