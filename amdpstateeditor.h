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
    void generateUI(gputypes *types);
public slots:
    void detectIndex();

private:
    Ui::amdPstateEditor *ui;
    struct pstate {
        QSlider *voltslider;
        QSlider *freqslider;
        QSpinBox *voltspinbox;
        QSpinBox *freqspinbox;
    };
    QVector <pstate> pstates;
};

#endif // AMDPSTATEEDITOR_H
