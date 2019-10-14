#pragma once

// Editor for selecting enumerated values

#include <QWidget>
#include <QVariant>
#include <QHBoxLayout>
#include <QComboBox>
#include <AssignableData.h>


class EnumEditor : public QWidget {
    Q_OBJECT
public:
    EnumEditor(QWidget *parent = nullptr);
    EnumEditor(QWidget *parent = nullptr, const AssignableData &data = nullptr);
    
    // Return selected string
    QString value();
    void setData(QStringList &strings);
private:
    QHBoxLayout *m_mainLayout;
    
    QStringList m_options;
    QComboBox *m_comboBox;
};
