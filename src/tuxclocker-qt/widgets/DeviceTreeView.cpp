#include "DeviceTreeView.hpp"
#include "qcheckbox.h"

#include <DragChartView.hpp>
#include <Globals.hpp>
#include <patterns.hpp>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QHeaderView>
#include <QSettings>
#include <Utils.hpp>

using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(AssignableProxy *)

DeviceTreeView::DeviceTreeView(QWidget *parent) : QTreeView(parent) {
	header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	connect(header(), &QHeaderView::sortIndicatorChanged,
	    [=](auto index, auto order) { saveSortOrder(index, order); });

	setSortingEnabled(true);
	setEditTriggers(SelectedClicked | EditKeyPressed);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(this, &QTreeView::collapsed, [=](auto &index) {
		// Traverse model, recursively suspend everything
		suspendChildren(index);
		// TODO: only save on exit
		// Difficulty has been recognizing 'exit' when a bunch of stuff isn't NULL
		saveCollapsed(model());
	});

	connect(this, &QTreeView::expanded, [=](auto &index) {
		// Traverse model, recursively resuming expanded nodes
		resumeChildren(index);
		// TODO: only save on exit
		saveCollapsed(model());
	});

	// Semi-hack: don't try to copy assignable settings from indices when loading profile
	auto app = static_cast<QApplication *>(QApplication::instance());
	connect(app, &QApplication::focusChanged, [=](QWidget *old, QWidget *now) {
		if (!now)
			return;
		auto functionEditor = Globals::g_functionEditor;
		// Don't lose selection when function editor is active
		// TODO: more proper way would be isAncestorOf but it thinks
		// the NodeSelector combobox isn't a child -_-
		auto editorActive = functionEditor && functionEditor->isVisible();
		// Workaround for same reason as above for EnumEditor delegate
		auto newTypeStr = now->metaObject()->className();
		// Seriously, WTF?
		auto isComboBox = QString{newTypeStr} == QString{"QComboBoxListView"};
		if (!editorActive && now && !this->isAncestorOf(now) && !isComboBox) {
			m_editSelection = {};
			m_editedIndex = QModelIndex{};
			clearSelection();
		}
	});
	m_delegate = new DeviceModelDelegate(this);

	setItemDelegate(m_delegate);
}

void DeviceTreeView::suspendChildren(const QModelIndex &index) {
	// Somehow can be true when we collapse nodes from cache
	if (!Globals::g_deviceModel)
		return;

	// TODO: readables aren't suspended when hidden and a connection using it stops
	QVector<QString> connectedReadablePaths;
	for (auto &conn : Globals::g_deviceModel->activeConnections())
		connectedReadablePaths.append(conn.dynamicReadablePath);

	auto cb = [=](QAbstractItemModel *model, const QModelIndex &index, int row) {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto dynProxyV = ifaceIndex.data(DeviceModel::DynamicReadableProxyRole);

		if (dynProxyV.isValid()) {
			auto proxy = qvariant_cast<DynamicReadableProxy *>(dynProxyV);
			// Don't suspend readables that are used in connections
			if (!connectedReadablePaths.contains(proxy->dbusPath()))
				proxy->suspend();
		}

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

bool assignableInfoEquals(AssignableInfo a, AssignableInfo b) {
	if (a.index() != b.index())
		return false;

	if (std::holds_alternative<RangeInfo>(a)) {
		auto rinfoA = std::get<RangeInfo>(a);
		auto rinfoB = std::get<RangeInfo>(b);
		if (rinfoA.index() != rinfoB.index())
			return false;

		if (std::holds_alternative<Range<int>>(rinfoA)) {
			auto rangeA = std::get<Range<int>>(rinfoA);
			auto rangeB = std::get<Range<int>>(rinfoB);

			return rangeA.max == rangeB.max && rangeA.min == rangeB.min;
		}
		if (std::holds_alternative<Range<double>>(rinfoA)) {
			auto rangeA = std::get<Range<double>>(rinfoA);
			auto rangeB = std::get<Range<double>>(rinfoB);

			return qFuzzyCompare(rangeA.max, rangeB.max) &&
			       qFuzzyCompare(rangeA.min, rangeB.min);
		}
	}
	if (std::holds_alternative<EnumerationVec>(a)) {
		auto enumsA = std::get<EnumerationVec>(a);
		auto enumsB = std::get<EnumerationVec>(b);
		if (enumsA.size() != enumsB.size())
			return false;

		for (int i = 0; i < enumsA.size(); i++) {
			if (enumsA[i].key != enumsB[i].key || enumsA[i].name != enumsB[i].name)
				return false;
		}
		return true;
	}

	return false;
}

bool indexApplicableForIndex(QModelIndex &orig, QModelIndex &other) {
	if (orig.column() != other.column())
		return false;

	auto origAssInfoV = orig.data(DeviceModel::AssignableRole);
	auto otherAssInfoV = other.data(DeviceModel::AssignableRole);
	if (!origAssInfoV.isValid() || !otherAssInfoV.isValid())
		return false;

	auto origAssData = qvariant_cast<AssignableItemData>(origAssInfoV);
	auto otherAssData = qvariant_cast<AssignableItemData>(otherAssInfoV);

	return assignableInfoEquals(origAssData.assignableInfo(), otherAssData.assignableInfo());
}

void DeviceTreeView::dataChanged(
    const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
	auto assData = topLeft.data(DeviceModel::AssignableRole);
	if (assData.isValid() && roles.contains(DeviceModel::AssignableRole)) {
		for (auto &index : m_editSelection) {
			// See if node is applicable to being set the same value
			// as the node the editor was opened for
			if (indexApplicableForIndex(m_editedIndex, index))
				model()->setData(index, assData, DeviceModel::AssignableRole);
		}
	}
	QTreeView::dataChanged(topLeft, bottomRight, roles);
}

void DeviceTreeView::saveCollapsed(QAbstractItemModel *model) {
	QStringList collapsed;
	auto cb = [&](auto *model, auto &index, int row) -> std::optional<QModelIndex> {
		if (!isExpanded(index)) {
			auto pathV = index.data(DeviceModel::NodePathRole);
			if (pathV.isValid())
				collapsed.append(pathV.toString());
		}

		return model->index(row, DeviceModel::NameColumn, index);
	};
	Utils::traverseModel(cb, model, QModelIndex{});

	QSettings cache{Utils::cacheFilePath(), QSettings::NativeFormat};
	cache.setValue("collapsedNodes", collapsed);
}

void DeviceTreeView::restoreCollapsed(QAbstractItemModel *model) {
	QSettings cache{Utils::cacheFilePath(), QSettings::NativeFormat};
	auto collapsed = qvariant_cast<QStringList>(cache.value("collapsedNodes"));

	expandAll();

	auto cb = [=](auto *model, auto &index, int row) -> std::optional<QModelIndex> {
		auto pathV = index.data(DeviceModel::NodePathRole);
		if (pathV.isValid() && collapsed.contains(pathV.toString())) {
			this->collapse(index);
		}
		return model->index(row, DeviceModel::NameColumn, index);
	};
	Utils::traverseModel(cb, model, QModelIndex{});
}

void DeviceTreeView::setModel(QAbstractItemModel *model) {
	QTreeView::setModel(model);

	restoreCollapsed(model);
	restoreSortOrder();
}

void DeviceTreeView::saveSortOrder(int column, Qt::SortOrder order) {
	if (column != DeviceModel::NameColumn)
		return;

	QSettings cache{Utils::cacheFilePath(), QSettings::NativeFormat};
	cache.setValue("sortOrder", order);
}

void DeviceTreeView::restoreSortOrder() {
	QSettings cache{Utils::cacheFilePath(), QSettings::NativeFormat};
	if (cache.contains("sortOrder")) {
		sortByColumn(DeviceModel::NameColumn,
		    static_cast<Qt::SortOrder>(cache.value("sortOrder").toInt()));
	}
}
