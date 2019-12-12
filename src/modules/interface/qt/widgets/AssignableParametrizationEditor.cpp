#include "AssignableParametrizationEditor.h"
#include <AssignableParametrizationData.h>

#include <QLabel>
#include <QDebug>

AssignableParametrizationEditor::AssignableParametrizationEditor(QWidget *parent) : AbstractExpandableItemEditor(parent) {
    setAutoFillBackground(true);
    
    setLayout(new QHBoxLayout);
    m_stackedWidget = new QStackedWidget;
    
    m_initialWidget = new QWidget;
    
    m_initialWidget->setLayout(new QHBoxLayout);
    m_initialWidget->layout()->setMargin(0);
    
    // Group enablement combobox and label together
    auto enableWidget = new QWidget;
    enableWidget->setLayout(new QHBoxLayout);
    enableWidget->layout()->setMargin(0);
    enableWidget->layout()->addWidget(new QLabel("Enabled"));
    enableWidget->layout()->addWidget((m_enabledCheckBox = new QCheckBox));
    // FIXME : this makes the layout unable to shrink
    enableWidget->layout()->setSizeConstraint(QLayout::SetFixedSize);
    
    m_initialWidget->layout()->addWidget(enableWidget);
    m_initialWidget->layout()->addWidget((m_editButton = new QPushButton("Edit")));
    
    m_stackedWidget->addWidget(m_initialWidget);
    m_stackedWidget->setCurrentWidget(m_initialWidget);
    
    m_stackedWidget->addWidget((m_editorWidget = new GraphEditor));
    
    connect(m_editButton, &QPushButton::clicked, [=]() {
        m_editorWidget->dragChartView()->setVector(m_parametrizationData.pointsVector());
        m_stackedWidget->setCurrentWidget(m_editorWidget);
        emit expansionSizeRequested(QSize(200, 300));
    });
    
    connect(m_editorWidget, &GraphEditor::cancelled, [=]() {
        m_stackedWidget->setCurrentWidget(m_initialWidget);
        emit expansionDisableRequested();
    });
    
    connect(m_editorWidget, &GraphEditor::saved, [=]() {
        m_stackedWidget->setCurrentWidget(m_initialWidget);
        emit expansionDisableRequested();
        m_parametrizationData.setPointsVector(m_editorWidget->dragChartView()->vector());
    });
    
    layout()->addWidget(m_stackedWidget);
    layout()->setMargin(0);
}

void AssignableParametrizationEditor::setData(AssignableParametrizationData &data) {
    m_parametrizationData = data;
    // Update y-axis label
    m_editorWidget->dragChartView()->yAxis()->setTitleText(m_parametrizationData.assignableData().name);
    // Set y-axis range
    switch (m_parametrizationData.assignableData().m_rangeInfo.range_data_type) {
        case TC_ASSIGNABLE_RANGE_DOUBLE:
            m_editorWidget->dragChartView()->yAxis()->setMin(m_parametrizationData.assignableData().m_rangeInfo.double_range.min);
            m_editorWidget->dragChartView()->yAxis()->setMax(m_parametrizationData.assignableData().m_rangeInfo.double_range.max);
            break;
        case TC_ASSIGNABLE_RANGE_INT:
            m_editorWidget->dragChartView()->yAxis()->setMin(m_parametrizationData.assignableData().m_rangeInfo.int_range.min);
            m_editorWidget->dragChartView()->yAxis()->setMax(m_parametrizationData.assignableData().m_rangeInfo.int_range.max);
            break;
        default:
            break;
    }
}
