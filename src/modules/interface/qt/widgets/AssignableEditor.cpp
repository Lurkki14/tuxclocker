#include "AssignableEditor.h"
#include "IntRangeEditor.h"

#include <QDebug>

AssignableEditor::AssignableEditor(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QGridLayout;
    
    m_title = new QLabel;
    m_title->setText("Select an editable node to edit it.");
    m_mainLayout->addWidget(m_title);
    
    m_editorStackedWidget = new QStackedWidget;
    
    m_noneEditor = new QWidget;
    m_intRangeEditor = new IntRangeEditor;
    m_enumEditor = new EnumEditor;
    
    m_editorStackedWidget->addWidget(m_noneEditor);
    m_editorStackedWidget->addWidget(m_intRangeEditor);
    m_editorStackedWidget->addWidget(m_enumEditor);
    
    m_editorStackedWidget->setCurrentWidget(m_noneEditor);
    m_mainLayout->addWidget(m_editorStackedWidget);
    
    setLayout(m_mainLayout);
}

AssignableEditor::~AssignableEditor() {
}

void AssignableEditor::setData(const QVariant& data) {
    
    m_assignableData = qvariant_cast<AssignableData>(data);
    
    // Change the title
    m_title->setText(m_assignableData.name);
    
    QStringList optList;
    // Switch to the correct subeditor
    switch (m_assignableData.m_valueCategory) {
        case TC_ASSIGNABLE_RANGE:
            m_intRangeEditor->setRange(m_assignableData.m_rangeInfo.int_range);
            m_editorStackedWidget->setCurrentWidget(m_intRangeEditor);
            break;
        case TC_ASSIGNABLE_ENUM:
            // Convert properties member to QStringList
            for (uint16_t i = 0; i < m_assignableData.m_enumInfo.property_count; i++) {
                QString opt(m_assignableData.m_enumInfo.properties[i]);
                optList.append(opt);
            }
            m_enumEditor->setData(optList);
            m_editorStackedWidget->setCurrentWidget(m_enumEditor);
            break;
        default:
            m_editorStackedWidget->setCurrentWidget(m_noneEditor);
            break;
    }
}
