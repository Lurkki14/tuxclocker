#pragma once

#include <Device.hpp>
#include <QVariant>

namespace TC = TuxClocker;

class AssignableItemData {
public:
	AssignableItemData() {m_enabled = false;}
	AssignableItemData(TC::Device::AssignableInfo info) : m_info(info) {
		m_enabled = false;
	}
	TC::Device::AssignableInfo assignableInfo() {return m_info;}
	void setValue(QVariant v) {m_targetValue = v;}
	QVariant value() {return m_targetValue;}
private:
	bool m_enabled;
	TC::Device::AssignableInfo m_info;
	QVariant m_targetValue;
};
Q_DECLARE_METATYPE(TC::Device::AssignableInfo)
