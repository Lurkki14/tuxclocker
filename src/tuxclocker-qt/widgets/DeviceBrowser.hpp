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
	DeviceBrowser(DeviceModel &model, QWidget *parent = nullptr);
private:
	DeviceModel &m_deviceModel;
	DeviceProxyModel *m_proxyModel;
	DeviceTreeView *m_treeView;
	FlagEditor<DeviceModel::InterfaceFlag> *m_flagEditor;
	QLabel *m_flagLabel;
	QPushButton *m_apply;
	QGridLayout *m_layout;
};
