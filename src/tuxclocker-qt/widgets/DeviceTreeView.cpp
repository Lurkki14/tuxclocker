#include "DeviceTreeView.hpp"
#include "qcheckbox.h"

#include <DragChartView.hpp>
#include <patterns.hpp>
#include <QCheckBox>
#include <QDebug>
#include <QHeaderView>
#include <Utils.hpp>

using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(AssignableProxy *)

DeviceTreeView::DeviceTreeView(QWidget *parent) : QTreeView(parent) {
	header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	setSortingEnabled(true);
	setEditTriggers(SelectedClicked | EditKeyPressed);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	connect(this, &QTreeView::collapsed, [=](auto &index) {
		// Traverse model, recursively suspend everything
		suspendChildren(index);
	});

	connect(this, &QTreeView::expanded, [=](auto &index) {
		// Traverse model, recursively resuming expanded nodes
		resumeChildren(index);
	});

	m_delegate = new DeviceModelDelegate(this);

	setItemDelegate(m_delegate);
}

void DeviceTreeView::suspendChildren(const QModelIndex &index) {
	auto cb = [=](QAbstractItemModel *model, const QModelIndex &index, int row) {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto dynProxyV = ifaceIndex.data(DeviceModel::DynamicReadableProxyRole);

		if (dynProxyV.isValid())
			qvariant_cast<DynamicReadableProxy *>(dynProxyV)->suspend();

		return model->index(row, DeviceModel::NameColumn, index);
	};
	Utils::traverseModel(cb, model(), index);
}

void DeviceTreeView::resumeChildren(const QModelIndex &index) {
	// Resume expanded nodes
	auto cb = [=](auto *model, auto &index, int row) -> std::optional<QModelIndex> {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto dynProxyV = ifaceIndex.data(DeviceModel::DynamicReadableProxyRole);

		if (dynProxyV.isValid() && isExpanded(index))
			qvariant_cast<DynamicReadableProxy *>(dynProxyV)->resume();

		// TODO: slightly wasteful since we will recurse into collapsed nodes too
		return model->index(row, DeviceModel::NameColumn, index);
	};
	Utils::traverseModel(cb, model(), index);
}
