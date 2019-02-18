#ifndef AMDPSTATEEDITOR_H
#define AMDPSTATEEDITOR_H

#include <QDialog>
#include <QWidget>
#include <QSlider>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include "gputypes.h"

namespace Ui {
class amdPstateEditor;
}

class amdPstateEditor : public QDialog
{
    Q_OBJECT

public:
    explicit amdPstateEditor(QWidget *parent = nullptr);
    ~amdPstateEditor();
public slots:
    void generateUI(gputypes *types);

private:
    Ui::amdPstateEditor *ui;
    //gputypes *types;
    //amd *amdptr;
};

#endif // AMDPSTATEEDITOR_H
