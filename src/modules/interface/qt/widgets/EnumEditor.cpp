#include "EnumEditor.h"
EnumEditor::EnumEditor(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QHBoxLayout;
    m_mainLayout->setMargin(0);
    
    m_comboBox = new QComboBox;
    m_mainLayout->addWidget(m_comboBox);
    
    // Combo box doesn't contain anything at this point
    m_comboBox->setDisabled(true);
    
    setLayout(m_mainLayout);
}

EnumEditor::EnumEditor(QWidget* parent, const AssignableData &data) : QWidget(parent) {
    m_mainLayout = new QHBoxLayout;
    m_mainLayout->setMargin(0);
    
    m_comboBox = new QComboBox;
    m_mainLayout->addWidget(m_comboBox);
    
    QStringList opts;
    // Construct the combo box entries
    for (uint16_t i = 0; i < data.m_enumInfo.property_count; i++) {
        opts.append(QString(data.m_enumInfo.properties[i]));
    }
    m_options = opts;
    m_comboBox->addItems(m_options);
    
    setLayout(m_mainLayout);
}

QString EnumEditor::value() {
    return m_comboBox->currentText();
}

void EnumEditor::setData(QStringList &strings) {
    // Remove old entries
    m_comboBox->clear();
        
    // Add the new options for the combo box
    m_options = strings;
    
    m_comboBox->addItems(m_options);
    m_comboBox->setEnabled(true);
}
