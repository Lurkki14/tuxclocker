#pragma once

#include <DeviceTreeView.hpp>
#include <DeviceModel.hpp>
#include <DeviceProxyModel.hpp>
#include <FlagEditor.hpp>
#include <FunctionEditor.hpp>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

// Class for viewing/editing the main tuxclocker tree
class DeviceBrowser : public QWidget {
public:
	DeviceBrowser(DeviceModel &model, QWidget *parent = nullptr)
			: QWidget(parent), m_deviceModel(model) {
		m_layout = new QGridLayout(this);
		m_proxyModel = new DeviceProxyModel(model, this);
		m_treeView = new DeviceTreeView;
		m_treeView->setModel(m_proxyModel);
		m_flagLabel = new QLabel("Showing:");
		m_apply = new QPushButton("Apply changes");
		m_apply->setEnabled(true);
		
		m_flagEditor = new FlagEditor(
			QVector({
				std::tuple(
					QString("Assignables"),
					DeviceModel::assignableIcon(),
					DeviceModel::Assignable
				),
				std::tuple(
					QString("Dynamic Values"),
					DeviceModel::dynamicReadableIcon(),
					DeviceModel::DynamicReadable
				),
				std::tuple(
					QString("Static Values"),
					DeviceModel::staticReadableIcon(),
					DeviceModel::StaticReadable
				)
			}), this);
		
		connect(m_apply, &QPushButton::pressed, &m_deviceModel,
			&DeviceModel::applyChanges);
		
		m_treeView->functionEditorRequested.connect([this] {
			auto f_editor = new FunctionEditor(m_deviceModel);
			f_editor->show();
		});
		
		m_flagEditor->setFlags(DeviceModel::AllInterfaces);
		
		m_flagEditor->flagsChanged.connect([=](auto flags) {
			m_proxyModel->setFlags(flags);
		});
		
		m_layout->addWidget(m_flagLabel, 0, 0);
		m_layout->addWidget(m_flagEditor, 0, 1);
		m_layout->addWidget(m_treeView, 1, 0, 1, 2);
		m_layout->addWidget(m_apply, 2, 0, 1, 2);
		
		setLayout(m_layout);
	}
	
private:
	DeviceModel &m_deviceModel;
	DeviceProxyModel *m_proxyModel;
	DeviceTreeView *m_treeView;
	FlagEditor<DeviceModel::InterfaceFlag> *m_flagEditor;
	QLabel *m_flagLabel;
	QPushButton *m_apply;
	QGridLayout *m_layout;
};
