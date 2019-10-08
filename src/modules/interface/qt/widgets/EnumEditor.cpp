#include "EnumEditor.h"

EnumEditor::EnumEditor(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QHBoxLayout;
    
    m_comboBox = new QComboBox;
    m_mainLayout->addWidget(m_comboBox);
    
    // Combo box doesn't contain anything at this point
    m_comboBox->setDisabled(true);
    
    setLayout(m_mainLayout);
}

void EnumEditor::setData(QStringList &strings) {
    // Remove old entries
    m_comboBox->clear();
        
    // Add the new options for the combo box
    m_options = strings;
    
    m_comboBox->addItems(m_options);
    m_comboBox->setEnabled(true);
}
