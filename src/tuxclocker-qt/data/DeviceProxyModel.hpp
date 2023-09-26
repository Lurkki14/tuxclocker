#pragma once

#include "DeviceModel.hpp"
#include <QDebug>
#include <QSortFilterProxyModel>

// Filters items from the device model based on the node interface type
class DeviceProxyModel : public QSortFilterProxyModel {
public:
	DeviceProxyModel(DeviceModel &model, QObject *parent = nullptr);
	DeviceModel::InterfaceFlags filterFlags() { return m_flags; }
	// Disable items that don't contain the interface in m_flags
	void setDisableFiltered(bool on) { m_disableFiltered = on; }
	void setFlags(DeviceModel::InterfaceFlags flags) {
		m_flags = flags;
		invalidateFilter();
	}
	// Don't show the interface icon (used in FunctionEditor)
	void setShowIcons(bool on) {
		m_showIcons = on;
		invalidateFilter();
	}
	/* TODO: Replace this with more generic solution if we want to hide the
	   name column too */
	void setShowValueColumn(bool on) {
		m_showValueColumn = on;
		invalidateFilter();
	}
protected:
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
		if (!m_showIcons && role == Qt::DecorationRole)
			return QVariant();
		return QSortFilterProxyModel::data(index, role);
	}
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
	bool filterAcceptsColumn(int sourceColumn, const QModelIndex &) const override {
		return !(!m_showValueColumn && sourceColumn == DeviceModel::InterfaceColumn);
	}
	// Optionally disable selection of items that don't have the wanted interface
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	// Overridden for showing items with children last/first
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
private:
	bool m_disableFiltered;
	bool m_showIcons;
	bool m_showValueColumn;
	DeviceModel::InterfaceFlags m_flags;
};
