#include "AssignableItem.hpp"

#include <DynamicReadableConnection.hpp>

Q_DECLARE_METATYPE(AssignableItemData)

void AssignableItem::setData(const QVariant &v, int role) {
	if (role == DeviceModel::AssignableRole) {
		auto data = v.value<AssignableItemData>().value();
		// Value is not empty
		if (data.isValid()) {
			updateText(v.value<AssignableItemData>());

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

void AssignableItem::updateText(AssignableItemData data) {
	if (data.value().canConvert<DynamicReadableConnectionData>()) {
		setText("(Parametrized)");
		return;
	}

	if (std::holds_alternative<RangeInfo>(data.assignableInfo())) {
		if (m_unit.has_value())
			setText(QString{"%1 %2"}.arg(data.value().toString(), m_unit.value()));
		else
			setText(data.value().toString());
		return;
	}

	if (std::holds_alternative<EnumerationVec>(data.assignableInfo())) {
		auto index = data.value().toUInt();
		auto enumVec = std::get<EnumerationVec>(data.assignableInfo());

		if (enumVec.size() - 1 >= index) {
			setText(QString::fromStdString(enumVec[index].name));
		} else {
			// This could arise from the settings being edited manually
			qWarning("Trying to set Enumeration with invalid index %u!", index);
			setText(QString::number(index));
		}
	}
}
