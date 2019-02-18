#include "amdpstateeditor.h"
#include "ui_amdpstateeditor.h"

amdPstateEditor::amdPstateEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::amdPstateEditor)
{
    ui->setupUi(this);
}

amdPstateEditor::~amdPstateEditor()
{
    delete ui;
}
void amdPstateEditor::generateUI(gputypes *types)
{
    QWidget *lower = new QWidget;
    QWidget *upper = new QWidget;
    QHBoxLayout *ulo = new QHBoxLayout;
    QHBoxLayout *llo = new QHBoxLayout;
    for (int i=0; i<types->GPUList[0].coreclocks.size(); i++) {
        pstate pstate;
        QGridLayout *glo = new QGridLayout;
        QLabel *voltlabel = new QLabel;
        QLabel *freqlabel = new QLabel;
        QLabel *pstatelabel = new QLabel;
        voltlabel->setText("mV");
        freqlabel->setText("MHz");
        pstatelabel->setText("Core pstate "+QString::number(i));
        QSlider *freqslider = new QSlider;
        QSlider *voltslider = new QSlider;
        QSpinBox *freqspinbox = new QSpinBox;
        QSpinBox *voltspinbox = new QSpinBox;
        connect(freqslider, SIGNAL(valueChanged(int value)), SLOT(detectIndex()));

        freqslider->setRange(types->GPUList[0].coreclocks[0], types->GPUList[0].coreclocks[types->GPUList[0].coreclocks.size()-1]);
        freqspinbox->setRange(types->GPUList[0].coreclocks[0], types->GPUList[0].coreclocks[types->GPUList[0].coreclocks.size()-1]);

        voltslider->setRange(types->GPUList[0].corevolts[0], types->GPUList[0].corevolts[types->GPUList[0].corevolts.size()-1]);
        voltspinbox->setRange(types->GPUList[0].corevolts[0], types->GPUList[0].corevolts[types->GPUList[0].corevolts.size()-1]);

        voltspinbox->setValue(types->GPUList[0].corevolts[i]);
        freqspinbox->setValue(types->GPUList[0].coreclocks[i]);

        glo->addWidget(pstatelabel);
        glo->addWidget(freqlabel, 1, 0);
        glo->addWidget(voltlabel, 1, 1);
        glo->addWidget(freqslider, 2, 0);
        glo->addWidget(voltslider, 2, 1);
        glo->addWidget(freqspinbox, 3, 0);
        glo->addWidget(voltspinbox, 3, 1);
        QWidget *freqsliderowdg = new QWidget;
        freqsliderowdg->setLayout(glo);
        llo->addWidget(freqsliderowdg);

        pstate.voltslider = voltslider;
        pstate.freqslider = freqslider;
        pstate.voltspinbox = voltspinbox;
        pstate.freqspinbox = freqspinbox;
        pstates.append(pstate);
    }

    for (int i=0; i<types->GPUList[0].memclocks.size(); i++) {
        pstate pstate;
        QGridLayout *glo = new QGridLayout;
        QLabel *voltlabel = new QLabel;
        QLabel *freqlabel = new QLabel;
        QLabel *pstatelabel = new QLabel;
        voltlabel->setText("mV");
        freqlabel->setText("MHz");
        pstatelabel->setText("Memory pstate "+QString::number(i));
        QSlider *freqslider = new QSlider;
        QSlider *voltslider = new QSlider;
        QSpinBox *freqspinbox = new QSpinBox;
        QSpinBox *voltspinbox = new QSpinBox;

        freqslider->setRange(types->GPUList[0].memclocks[0], types->GPUList[0].memclocks[types->GPUList[0].memclocks.size()-1]);
        freqspinbox->setRange(types->GPUList[0].coreclocks[0], types->GPUList[0].coreclocks[types->GPUList[0].memclocks.size()-1]);

        voltslider->setRange(types->GPUList[0].memvolts[0], types->GPUList[0].memvolts[types->GPUList[0].memvolts.size()-1]);
        voltspinbox->setRange(types->GPUList[0].memvolts[0], types->GPUList[0].memvolts[types->GPUList[0].memvolts.size()-1]);

        voltspinbox->setValue(types->GPUList[0].memvolts[i]);
        freqspinbox->setValue(types->GPUList[0].memclocks[i]);

        glo->addWidget(pstatelabel);
        glo->addWidget(freqlabel, 1, 0);
        glo->addWidget(voltlabel, 1, 1);
        glo->addWidget(freqslider, 2, 0);
        glo->addWidget(voltslider, 2, 1);
        glo->addWidget(freqspinbox, 3, 0);
        glo->addWidget(voltspinbox, 3, 1);
        QWidget *freqsliderowdg = new QWidget;
        freqsliderowdg->setLayout(glo);
        ulo->addWidget(freqsliderowdg);

        pstate.voltslider = voltslider;
        pstate.freqslider = freqslider;
        pstate.voltspinbox = voltspinbox;
        pstate.freqspinbox = freqspinbox;
        pstates.append(pstate);
    }
    lower->setLayout(ulo);
    upper->setLayout(llo);

    QVBoxLayout *mainlo = new QVBoxLayout(this);
    mainlo->addWidget(upper);
    mainlo->addWidget(lower);
    ui->centralWidget->setLayout(mainlo);
}
void amdPstateEditor::detectIndex()
{
    for (int i=0; i<pstates.size(); i++) {
        if (pstates[i].freqslider->isSliderDown()) {
            qDebug() << "slider" << i << "is active";
        }
    }
}
