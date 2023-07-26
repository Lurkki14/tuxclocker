#pragma once

#include <Device.hpp>
#include <QVariant>

namespace TC = TuxClocker;

class AssignableItemData {
public:
	AssignableItemData() { m_enabled = false; }
	AssignableItemData(TC::Device::AssignableInfo info, std::optional<QString> unit)
	    : m_info(info) {
		m_unit = unit;
		m_enabled = false;
	}
	TC::Device::AssignableInfo assignableInfo() { return m_info; }
	bool committal() { return m_enabled; }
	// Whether or not the set value shall be applied. Doesn't reset or change it.
	void setCommittal(bool on) { m_enabled = on; }
	void setValue(QVariant v) { m_targetValue = v; }
	QVariant value() { return m_targetValue; }
	std::optional<QString> unit() { return m_unit; }
private:
	bool m_enabled;
	TC::Device::AssignableInfo m_info;
	std::optional<QString> m_unit;
	QVariant m_targetValue;
};
