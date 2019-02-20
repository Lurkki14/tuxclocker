#include "amdpstateeditor.h"
#include "ui_amdpstateeditor.h"

amdPstateEditor::amdPstateEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::amdPstateEditor)
{
    ui->setupUi(this);
    generateUI();
}

amdPstateEditor::~amdPstateEditor()
{
    delete ui;
}
void amdPstateEditor::grabPointer(gputypes *newtypes)
{
    types = newtypes;
}
void amdPstateEditor::generateUI()
{
    qDebug() << types->gpuCount << "gpus in pointer";
    QWidget *lower = new QWidget;
    QWidget *upper = new QWidget;
    QHBoxLayout *ulo = new QHBoxLayout;
    QHBoxLayout *llo = new QHBoxLayout;
    for (int i=0; i<types->GPUList[0].coreclocks.size(); i++) {
        corePstate state;
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
        connect(freqslider, SIGNAL(valueChanged(int)), freqspinbox, SLOT(setValue(int)));
        connect(freqspinbox, SIGNAL(valueChanged(int)), freqslider, SLOT(setValue(int)));
        connect(voltspinbox, SIGNAL(valueChanged(int)), voltslider, SLOT(setValue(int)));
        connect(voltslider, SIGNAL(valueChanged(int)), voltspinbox, SLOT(setValue(int)));

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
        ulo->addWidget(freqsliderowdg);

        state.voltspinbox = voltspinbox;
        state.freqspinbox = freqspinbox;
        corePstates.append(state);
    }

    for (int i=0; i<types->GPUList[0].memclocks.size(); i++) {
        memPstate state;
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
        connect(freqslider, SIGNAL(valueChanged(int)), freqspinbox, SLOT(setValue(int)));
        connect(freqspinbox, SIGNAL(valueChanged(int)), freqslider, SLOT(setValue(int)));
        connect(voltspinbox, SIGNAL(valueChanged(int)), voltslider, SLOT(setValue(int)));
        connect(voltslider, SIGNAL(valueChanged(int)), voltspinbox, SLOT(setValue(int)));

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
        llo->addWidget(freqsliderowdg);
        state.voltspinbox = voltspinbox;
        state.freqspinbox = freqspinbox;
        memPstates.append(state);
    }
    // Add an apply button
    QPushButton *applyButton = new QPushButton;
    connect(applyButton, SIGNAL(clicked()), SLOT(applyValues()));
    applyButton->setText("Apply values");
    llo->addWidget(applyButton);

    lower->setLayout(llo);
    upper->setLayout(ulo);

    QVBoxLayout *mainlo = new QVBoxLayout(this);
    mainlo->addWidget(upper);
    mainlo->addWidget(lower);
    ui->centralWidget->setLayout(mainlo);
}
bool amdPstateEditor::applyValues()
{
    qDebug("Applying values");
    QProcess proc;
    QString volt;
    QString freq;
    // Apply core pstates
    for (int i=0; i<corePstates.size(); i++) {
        if ((corePstates[i].freqspinbox->value() != types->GPUList[0].coreclocks[i]) || (corePstates[i].voltspinbox->value() != types->GPUList[0].corevolts[i])) {
            volt = QString::number(corePstates[i].freqspinbox->value());
            freq = QString::number(corePstates[i].voltspinbox->value());
            proc.start("/bin/sh -c \"pkexec echo \"s "+ volt +" "+ freq +"\" "+"> /sys/class/drm/card"+QString::number(types->GPUList[0].fsindex)+"/device/pp_od_clk_voltage\"");
            proc.waitForFinished();
        }
    }
    // Apply memory pstates
    for (int i=0; i<memPstates.size(); i++) {
        if ((corePstates[i].freqspinbox->value() != types->GPUList[0].coreclocks[i]) || (corePstates[i].voltspinbox->value() != types->GPUList[0].corevolts[i])) {
            volt = QString::number(corePstates[i].freqspinbox->value());
            freq = QString::number(corePstates[i].voltspinbox->value());
            proc.start("/bin/sh -c \"pkexec echo \"s "+ volt +" "+ freq +"\" "+"> /sys/class/drm/card"+QString::number(types->GPUList[0].fsindex)+"/device/pp_od_clk_voltage\"");
            proc.waitForFinished();
        }
    }
    return true;
}
