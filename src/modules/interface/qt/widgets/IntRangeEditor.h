#pragma once

// Widget for editing TC_ASSIGNABLE_RANGE_INT nodes

#include <tc_assignable.h>

#include <QWidget>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>

class IntRangeEditor : public QWidget {
    Q_OBJECT
public:
    IntRangeEditor(QWidget *parent = nullptr);
    //IntRangeEditor(QWidget *parent = nullptr, const tc_assignable_range_int_t *range = nullptr);
    
    void setRange(const tc_assignable_range_int_t &range);
private:
    QHBoxLayout *m_mainLayout;
    QSlider *m_slider;
    QSpinBox *m_spinBox;
    
    tc_assignable_range_int_t m_range;
};
