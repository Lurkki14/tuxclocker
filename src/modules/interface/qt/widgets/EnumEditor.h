#pragma once

// Editor for selecting enumerated values

#include <QWidget>
#include <QHBoxLayout>
#include <QComboBox>

class EnumEditor : public QWidget {
    Q_OBJECT
public:
    EnumEditor(QWidget *parent = nullptr);
    
    void setData(QStringList &strings);
private:
    QHBoxLayout *m_mainLayout;
    
    QStringList m_options;
    QComboBox *m_comboBox;
};
