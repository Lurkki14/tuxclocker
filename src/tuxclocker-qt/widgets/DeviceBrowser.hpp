#pragma once

#include <DeviceTreeView.hpp>
#include <DeviceModel.hpp>
#include <QGridLayout>
#include <QPushButton>
#include <QWidget>

// Class for viewing/editing the main tuxclocker tree
class DeviceBrowser : public QWidget {
public:
	DeviceBrowser(DeviceModel &model,
			QWidget *parent = nullptr) : QWidget(parent), m_deviceModel(model) {
		m_layout = new QGridLayout(this);
		m_treeView = new DeviceTreeView(model);
		m_apply = new QPushButton("Apply changes");
		m_apply->setEnabled(true);
		
		connect(m_apply, &QPushButton::pressed, &m_deviceModel, &DeviceModel::applyChanges);
		
		m_layout->addWidget(m_treeView, 0, 0);
		m_layout->addWidget(m_apply, 1, 0);
		
		setLayout(m_layout);
	}
	
private:
	DeviceModel &m_deviceModel;
	DeviceTreeView *m_treeView;
	QPushButton *m_apply;
	QGridLayout *m_layout;
};
