#pragma once

#include <QWidget>
#include <QLayout>

class AssignableEditor : public QWidget {
    Q_OBJECT
public:
    AssignableEditor(QWidget *parent = nullptr);
    ~AssignableEditor();
private:
    QGridLayout *m_mainLayout;
};
