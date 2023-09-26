#include "EnumEditor.h"
#include <QDebug>

EnumEditor::EnumEditor(QWidget *parent) : AbstractAssignableEditor(parent) {
    m_mainLayout = new QHBoxLayout;
    m_mainLayout->setMargin(0);
    
    m_comboBox = new QComboBox;
    m_mainLayout->addWidget(m_comboBox);
    
    // Combo box doesn't contain anything at this point
    m_comboBox->setDisabled(true);
    
    setLayout(m_mainLayout);
}

QVariant EnumEditor::value() {
    return m_comboBox->currentIndex();
}

QString EnumEditor::text() {
    return m_comboBox->currentText();
}

void EnumEditor::setValue(QVariant value) {
    m_comboBox->setCurrentIndex(value.toInt());
}

void EnumEditor::setAssignableData(const AssignableData &data) {
    // Remove old entries
    m_comboBox->clear();
    
    // Convert char** to QStringList
    QStringList newOpts;
    for (uint16_t i = 0; i < data.m_enumInfo.property_count; i++) {
        newOpts.append(QString(data.m_enumInfo.properties[i]));
    }
    
    // Add the new options for the combo box
    m_options = newOpts;
    
    m_comboBox->addItems(m_options);
    m_comboBox->setEnabled(true);
}
