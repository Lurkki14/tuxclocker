#pragma once

#include "AbstractAssignableEditor.hpp"

#include <Device.hpp>
#include <QSlider>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QWidget>

namespace TC = TuxClocker;

class IntRangeEditor : public AbstractAssignableEditor {
public:
	IntRangeEditor(QWidget *parent = nullptr) : AbstractAssignableEditor(parent) {
		setAutoFillBackground(true);

		m_layout = new QHBoxLayout(this);
		m_layout->setMargin(0);
		m_slider = new QSlider(Qt::Horizontal);
		m_spinBox = new QSpinBox;

		m_layout->addWidget(m_slider);
		m_layout->addWidget(m_spinBox);

		setLayout(m_layout);

		connect(m_slider, &QSlider::rangeChanged, m_spinBox, &QSpinBox::setRange);
		connect(m_slider, &QSlider::valueChanged, m_spinBox, &QSpinBox::setValue);
		connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_slider,
		    &QSlider::setValue);
	}
	IntRangeEditor(TC::Device::Range<int> range, QWidget *parent = nullptr)
	    : IntRangeEditor(parent) {
		setRange(range);
	}

	virtual QVariant assignableData() override { return m_slider->value(); }
	virtual QString displayData() override { return QString::number(m_slider->value()); }
	virtual void setAssignableData(QVariant data) override { m_slider->setValue(data.toInt()); }
	void setRange(TC::Device::Range<int> range) { m_slider->setRange(range.min, range.max); }
	int value() { return m_slider->value(); }
private:
	QHBoxLayout *m_layout;
	QSlider *m_slider;
	QSpinBox *m_spinBox;
};
