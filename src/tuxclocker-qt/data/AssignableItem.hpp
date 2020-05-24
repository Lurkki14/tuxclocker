#pragma once

//#include "DeviceModel.hpp"

#include <QObject>
#include <QStandardItem>

// Class for forwarding changes in DeviceModel's assignables

// Forgive me for the sin of multiple inheritance
class AssignableItem : public QObject, public QStandardItem {
public:
	AssignableItem(QObject *parent = nullptr) : QObject(parent), QStandardItem() {
	}
	void setData(const QVariant &v, int role = Qt::UserRole + 1) {
		//if (role == DeviceModel::AssignableRole) ;
	}
signals:
	void assignableDataChanged(QVariant value);
private:
	Q_OBJECT
};
