#include "GraphEditor.h"

#include <QSizePolicy>

GraphEditor::GraphEditor(QWidget *parent) : QWidget(parent) {
    m_layout = new QGridLayout;
    
    // Layout is 4 x 2 layout with buttons spanning the lower right  of the second row
    
    m_layout->addWidget((m_dragChartView = new DragChartView), 0, 0, 1, 4);
    m_dragChartView->chart()->setMargins(QMargins(0, 0, 0, 0));
    
    m_layout->addWidget((m_saveButton = new QPushButton("Save")), 1, 2, 1, 1);
    m_layout->addWidget((m_cancelButton = new QPushButton("Cancel")), 1, 3, 1, 1);
    
    // Minimize button height
    QSizePolicy buttonPolicy(QSizePolicy::Preferred, QSizePolicy::Minimum, QSizePolicy::PushButton);
    m_saveButton->setSizePolicy(buttonPolicy);
    m_cancelButton->setSizePolicy(buttonPolicy);
    
    connect(m_cancelButton, &QPushButton::clicked, [=]() {
        emit cancelled();
    });
    
    setLayout(m_layout);
}
