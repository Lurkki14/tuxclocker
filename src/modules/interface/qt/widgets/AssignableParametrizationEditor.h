#pragma once

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>

class AssignableParametrizationEditor : public QWidget {
public:
    AssignableParametrizationEditor(QWidget *parent = nullptr);
private:
    QCheckBox *m_enabledCheckBox;
    QPushButton *m_editButton;
};
