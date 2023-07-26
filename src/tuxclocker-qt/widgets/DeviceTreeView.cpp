#include "DeviceTreeView.hpp"
#include "qcheckbox.h"

#include <DragChartView.hpp>
#include <patterns.hpp>
#include <QCheckBox>
#include <QDebug>
#include <QHeaderView>

using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(AssignableProxy *)

DeviceTreeView::DeviceTreeView(QWidget *parent) : QTreeView(parent) {
	header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	setSortingEnabled(true);
	setEditTriggers(SelectedClicked | EditKeyPressed);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QTreeView::customContextMenuRequested, [this](QPoint point) {
		auto index = indexAt(point);
		auto data = index.data(DeviceModel::AssignableRole);
		auto proxyData = index.data(DeviceModel::AssignableProxyRole);
		QMenu menu;
		QCheckBox checkBox("Enable connection");
		QAction editConn("Edit connection...");
		auto enableConn = new QWidgetAction(&menu);
		enableConn->setDefaultWidget(&checkBox);
		menu.addActions({&editConn, enableConn});
		if (data.canConvert<AssignableItemData>() &&
		    proxyData.canConvert<AssignableProxy *>()) {
			functionEditorRequested(index);
			/*auto a_data = data.value<AssignableItemData>();
			auto proxy = proxyData.value<AssignableProxy*>();
			match(a_data.assignableInfo()) (
				pattern(as<RangeInfo>(arg)) = [this, &menu, proxy, &editConn](auto
			ri) {
					//functionEditorRequested(*proxy, ri);
					connect(&editConn, &QAction::triggered, [this, proxy, ri] {
						functionEditorRequested(*proxy, ri);
					});
					menu.exec(QCursor::pos());
				},
				pattern(_) = []{}
			);*/
		}
	});
	m_delegate = new DeviceModelDelegate(this);

	setItemDelegate(m_delegate);
}
