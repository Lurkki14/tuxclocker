#include "AssignableEditor.h"

AssignableEditor::AssignableEditor(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QGridLayout;
    
    setLayout(m_mainLayout);
}

AssignableEditor::~AssignableEditor() {
}
