#pragma once

#include "DeviceModel.hpp"
#include <QSortFilterProxyModel>

// Filters items from the device model based on the node interface type
class DeviceProxyModel : public QSortFilterProxyModel {
public:
	DeviceProxyModel(DeviceModel &model, QObject *parent = nullptr);
	DeviceModel::InterfaceFlags filterFlags() {return m_flags;}
	void setFlags(DeviceModel::InterfaceFlags flags) {
		m_flags = flags;
		invalidateFilter();
	}
protected:
	bool filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const override;
private:
	DeviceModel::InterfaceFlags m_flags;
};
