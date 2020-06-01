#pragma once

#include "DeviceModel.hpp"

#include <QObject>
#include <QStandardItem>

// Class for forwarding changes in DeviceModel's assignables

// Forgive me for the sin of multiple inheritance
class AssignableItem : public QObject, public QStandardItem {
public:
	AssignableItem(QObject *parent = nullptr) : QObject(parent), QStandardItem() {
	}
	bool committal() {return m_committed;}
	// Whether or not the set value shall be applied. Doesn't reset or change it.
	void setCommittal(bool on) {m_committed = on;}
	void setData(const QVariant &v, int role = Qt::UserRole + 1);
signals:
	void assignableDataChanged(QVariant value);
private:
	Q_OBJECT
	
	bool m_committed = false;
};
