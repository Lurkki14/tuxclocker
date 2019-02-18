#ifndef AMDPSTATEEDITOR_H
#define AMDPSTATEEDITOR_H

#include <QDialog>
#include <QWidget>
<<<<<<< HEAD
#include <QSlider>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include "gputypes.h"
=======
#include "gputypes.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QLayout>
>>>>>>> 09ca6b3c9f828481d3690c9999546c2d9785d9bd

namespace Ui {
class amdPstateEditor;
}

class amdPstateEditor : public QDialog
{
    Q_OBJECT

public:
    explicit amdPstateEditor(QWidget *parent = nullptr);
    ~amdPstateEditor();
<<<<<<< HEAD
public slots:
=======
>>>>>>> 09ca6b3c9f828481d3690c9999546c2d9785d9bd
    void generateUI(gputypes *types);

private:
    Ui::amdPstateEditor *ui;
<<<<<<< HEAD
    //gputypes *types;
    //amd *amdptr;
=======
>>>>>>> 09ca6b3c9f828481d3690c9999546c2d9785d9bd
};

#endif // AMDPSTATEEDITOR_H
