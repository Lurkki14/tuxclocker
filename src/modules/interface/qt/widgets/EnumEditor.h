#pragma once

// Editor for selecting enumerated values

#include <QWidget>
#include <QVariant>
#include <QHBoxLayout>
#include <QComboBox>

#include <AssignableData.h>
#include "AbstractAssignableEditor.h"

class EnumEditor : public AbstractAssignableEditor {
    Q_OBJECT
public:
    EnumEditor(QWidget *parent = nullptr);
    
    // Return selected string
    QVariant value();
    void setValue(QVariant value) {}
    void setAssignableData(const AssignableData &data);
private:
    QHBoxLayout *m_mainLayout;
    
    QStringList m_options;
    QComboBox *m_comboBox;
};
