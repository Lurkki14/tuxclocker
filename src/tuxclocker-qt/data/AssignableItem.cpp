#include "AssignableItem.hpp"

Q_DECLARE_METATYPE(AssignableItemData)

void AssignableItem::setData(const QVariant &v, int role) {
	if (role == DeviceModel::AssignableRole) {
		auto data = v.value<AssignableItemData>().value();
		// Value is not empty
		if (data.isValid()) {
			QVariant vr;
			vr.setValue(data);
			emit assignableDataChanged(vr);
		}
	}
	if (role == Qt::CheckStateRole) {
		bool state = v.toBool();
		emit committalChanged(state);
	}
	QStandardItem::setData(v, role);
}
