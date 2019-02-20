#ifndef AMDPSTATEEDITOR_H
#define AMDPSTATEEDITOR_H

#include <QDialog>
#include <QWidget>
#include <QSlider>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
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
    void grabPointer(gputypes *newtypes);
    void generateUI();

private:
    Ui::amdPstateEditor *ui;
    // These are used for getting the values out of the sliders when applying, only a slider or spinbox is required, since they are synced
    struct memPstate {
        QSpinBox *voltspinbox;
        QSpinBox *freqspinbox;
    };
    struct corePstate {
        QSpinBox *voltspinbox;
        QSpinBox *freqspinbox;
    };
    QVector <corePstate> corePstates;
    QVector <memPstate> memPstates;
    gputypes *types;
private slots:
    bool applyValues();
};

#endif // AMDPSTATEEDITOR_H
