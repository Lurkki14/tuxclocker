#include "IntRangeEditor.h"

IntRangeEditor::IntRangeEditor(QWidget *parent) : AbstractAssignableEditor(parent) {
    m_mainLayout = new QHBoxLayout;
    m_mainLayout->setMargin(0);
    
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 0);
    m_mainLayout->addWidget(m_slider);
    
    m_spinBox = new QSpinBox;
    m_spinBox->setRange(0, 0);
    m_mainLayout->addWidget(m_spinBox);
    
    connect(m_slider, &QSlider::valueChanged, m_spinBox, &QSpinBox::setValue);
    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_slider, &QSlider::setValue);
    
    connect(m_slider, &QSlider::rangeChanged, m_spinBox, &QSpinBox::setRange);
    
    // No range specified at this point, so disable them
    m_slider->setDisabled(true);
    m_slider->setDisabled(true);
    
    setLayout(m_mainLayout);
    // Avoids the display text from overlapping during editing
    setAutoFillBackground(true);
}

QVariant IntRangeEditor::value() {
    return m_slider->value();
}

QString IntRangeEditor::text() {
    return QString::number(m_slider->value());
}

void IntRangeEditor::setValue(QVariant value) {
    m_slider->setValue(value.toInt());
}

void IntRangeEditor::setAssignableData(const AssignableData &data) {
    m_slider->setEnabled(true);
    m_spinBox->setEnabled(true);
    
    m_slider->setRange(data.m_rangeInfo.int_range.min, data.m_rangeInfo.int_range.max);
}

void IntRangeEditor::setRange(const tc_assignable_range_int_t &range) {
    m_range = range;
    
    m_slider->setEnabled(true);
    m_spinBox->setEnabled(true);
    
    m_slider->setRange(range.min, range.max);
}
