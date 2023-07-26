#pragma once

#include "AbstractAssignableEditor.hpp"

#include <Device.hpp>
#include <tgmath.h>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QSlider>

namespace TC = TuxClocker;

class DoubleRangeEditor : public AbstractAssignableEditor {
public:
	DoubleRangeEditor(QWidget *parent = nullptr) : AbstractAssignableEditor(parent) {
		setAutoFillBackground(true);
		m_layout = new QHBoxLayout(this);
		m_spinBox = new QDoubleSpinBox;
		m_slider = new QSlider(Qt::Horizontal);

		connect(m_slider, &QSlider::rangeChanged, [this](int min, int max) {
			m_spinBox->setRange(toDouble(min), toDouble(max));
		});
		connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		    [this](double val) { m_slider->setValue(toInt(val)); });
		connect(m_slider, &QSlider::valueChanged,
		    [this](int val) { m_spinBox->setValue(toDouble(val)); });

		m_layout->setMargin(0);
		m_layout->addWidget(m_slider);
		m_layout->addWidget(m_spinBox);
		setLayout(m_layout);
	}
	DoubleRangeEditor(TC::Device::Range<double> range, QWidget *parent = nullptr)
	    : DoubleRangeEditor(parent) {
		setRange(range);
	}
	virtual QVariant assignableData() override { return m_spinBox->value(); }
	virtual QString displayData() override { return QString::number(m_spinBox->value()); }
	virtual void setAssignableData(QVariant data) override {
		m_spinBox->setValue(data.toDouble());
	}
	void setRange(TC::Device::Range<double> range) {
		m_slider->setRange(toInt(range.min), toInt(range.max));
	}
private:
	// How many digits are displayed after the dot
	constexpr int doubleDecimals() { return 2; }
	double toDouble(int i) { return static_cast<double>(i) / pow(10, doubleDecimals()); }
	int toInt(double d) { return static_cast<int>(d * pow(10, doubleDecimals())); }

	QDoubleSpinBox *m_spinBox;
	QHBoxLayout *m_layout;
	QSlider *m_slider;
};
