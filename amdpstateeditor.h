#ifndef AMDPSTATEEDITOR_H
#define AMDPSTATEEDITOR_H

#include <QDialog>
#include <QWidget>
#include "gputypes.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QLayout>

namespace Ui {
class amdPstateEditor;
}

class amdPstateEditor : public QDialog
{
    Q_OBJECT

public:
    explicit amdPstateEditor(QWidget *parent = nullptr);
    ~amdPstateEditor();
    void generateUI(gputypes *types);

private:
    Ui::amdPstateEditor *ui;
};

#endif // AMDPSTATEEDITOR_H
