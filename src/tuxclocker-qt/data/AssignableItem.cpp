#include "AssignableItem.hpp"

Q_DECLARE_METATYPE(AssignableItemData)

void AssignableItem::setData(const QVariant &v, int role) {
	if (role == DeviceModel::AssignableRole) {
		auto data = v.value<AssignableItemData>().value();
		// Value is empty
		if (data.isValid()) {
			QVariant vr;
			vr.setValue(data);
			emit assignableDataChanged(vr);
		}
	}
	QStandardItem::setData(v, role);
}
