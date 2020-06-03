#pragma once

#include "AbstractAssignableEditor.hpp"

#include <Device.hpp>
#include <QComboBox>
#include <QStandardItemModel>

class EnumEditor : public AbstractAssignableEditor {
public:
	EnumEditor(QWidget *parent = nullptr);
	EnumEditor(TuxClocker::Device::EnumerationVec enums, QWidget *parent = nullptr);
	virtual QVariant assignableData() override;
	virtual QString displayData() override;
	virtual void setAssignableData(QVariant data) override;
private:
	QComboBox *m_comboBox;
	QStandardItemModel m_model;
};
