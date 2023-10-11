#include "AssignableItem.hpp"

#include <DynamicReadableConnection.hpp>
#include <libintl.h>

#define _(String) gettext(String)

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
		m_committed = state;
		emit committalChanged(state);
	}
	QStandardItem::setData(v, role);
}

QString AssignableItem::appendUnit(QString text) {
	if (m_unit.has_value())
		return QString{"%1 %2"}.arg(text, m_unit.value());
	return text;
}

void AssignableItem::setCurrentValueText(QString text) {
	m_currentValueText = text;

	QString targetText;
	// Don't show unit when target is parametrization
	if (m_currentTargetData.parametrizationText.has_value()) {
		targetText = m_currentTargetData.parametrizationText.value();
	} else if (m_currentTargetData.valueText.has_value()) {
		targetText = appendUnit(m_currentTargetData.valueText.value());
	} else {
		setText(appendUnit(text));
		return;
	}
	setText(QString{"%1 -> %2"}.arg(appendUnit(text), targetText));
}

void AssignableItem::applyTargetText() {
	if (m_currentTargetData.valueText.has_value()) {
		auto newCurrent = m_currentTargetData.valueText.value();
		setText(appendUnit(newCurrent));
		m_currentValueText = newCurrent;
	}

	m_currentTargetData.valueText = std::nullopt;
	m_currentTargetData.parametrizationText = std::nullopt;
}

void AssignableItem::clearTargetText() {
	updateText(m_currentValueText);
	m_currentTargetData.valueText = std::nullopt;
	m_currentTargetData.parametrizationText = std::nullopt;
}

void AssignableItem::updateText(QString &text) { setText(appendUnit(text)); }

void AssignableItem::updateText(AssignableItemData data) {
	QString targetText;
	if (data.value().canConvert<DynamicReadableConnectionData>()) {
		// Include possible unit
		auto target = _("(Parametrized)");
		m_currentTargetData.parametrizationText = target;
		targetText = target;
		goto ret;
	}

	if (std::holds_alternative<RangeInfo>(data.assignableInfo())) {
		targetText = appendUnit(data.value().toString());
		m_currentTargetData.valueText = data.value().toString();
		goto ret;
	}

	if (std::holds_alternative<EnumerationVec>(data.assignableInfo())) {
		auto index = data.value().toUInt();
		auto enumVec = std::get<EnumerationVec>(data.assignableInfo());

		if (enumVec.size() - 1 >= index) {
			targetText = QString::fromStdString(enumVec[index].name);
			m_currentTargetData.valueText = QString::fromStdString(enumVec[index].name);
		} else {
			// This could arise from the settings being edited manually
			qWarning("Trying to set Enumeration with invalid index %u!", index);
			m_currentTargetData.valueText = QString::number(index);
			targetText = QString::number(index);
		}
	}
ret:
	setText(QString{"%1 -> %2"}.arg(appendUnit(m_currentValueText), targetText));
}
