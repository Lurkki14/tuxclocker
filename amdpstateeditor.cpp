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
void amdPstateEditor::generateUI(gputypes *newtypes, int GPUIndex)
{
    types = newtypes;
    gpuidx = GPUIndex;
    QWidget *lower = new QWidget;
    QWidget *upper = new QWidget;
    QHBoxLayout *ulo = new QHBoxLayout;
    QHBoxLayout *llo = new QHBoxLayout;

    for (int i=0; i<newtypes->GPUList[gpuidx].coreclocks.size(); i++) {
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

        freqslider->setRange(newtypes->GPUList[gpuidx].minCoreClkLimit, newtypes->GPUList[gpuidx].maxCoreClkLimit);
        freqspinbox->setRange(newtypes->GPUList[gpuidx].minCoreClkLimit, newtypes->GPUList[gpuidx].maxCoreClkLimit);

        voltslider->setRange(newtypes->GPUList[gpuidx].minVoltageLimit, newtypes->GPUList[gpuidx].maxVoltageLimit);
        voltspinbox->setRange(newtypes->GPUList[gpuidx].minVoltageLimit, newtypes->GPUList[gpuidx].maxVoltageLimit);

        voltspinbox->setValue(newtypes->GPUList[gpuidx].corevolts[i]);
        freqspinbox->setValue(newtypes->GPUList[gpuidx].coreclocks[i]);

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

    for (int i=0; i<newtypes->GPUList[gpuidx].memclocks.size(); i++) {
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

        freqslider->setRange(newtypes->GPUList[gpuidx].minMemClkLimit, newtypes->GPUList[gpuidx].maxMemClkLimit);
        freqspinbox->setRange(newtypes->GPUList[gpuidx].minMemClkLimit, newtypes->GPUList[gpuidx].maxMemClkLimit);

        voltslider->setRange(newtypes->GPUList[gpuidx].minVoltageLimit, newtypes->GPUList[gpuidx].maxVoltageLimit);
        voltspinbox->setRange(newtypes->GPUList[gpuidx].minVoltageLimit, newtypes->GPUList[gpuidx].maxVoltageLimit);

        voltspinbox->setValue(newtypes->GPUList[gpuidx].memvolts[i]);
        freqspinbox->setValue(newtypes->GPUList[gpuidx].memclocks[i]);

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

    // Load the existing changes from settings
    QSettings settings("tuxclocker");
    QString profile = settings.value("currentProfile").toString();
    QString UUID = settings.value("latestUUID").toString();
    settings.beginGroup(profile);
    settings.beginGroup(UUID);
    // Read memory pstates
    settings.beginGroup("memPstates");
    // Get the indices of pstates
    QStringList memPstateIndices = settings.childGroups();
    for (int i=0; i<memPstateIndices.size(); i++) {
        settings.beginGroup(memPstateIndices[i]);
        // Set the appropriate slider values for the pstate
        int frequency = settings.value("frequency").toInt();
        int voltage = settings.value("voltage").toInt();
        memPstates[memPstateIndices[i].toInt()].freqspinbox->setValue(frequency);
        memPstates[memPstateIndices[i].toInt()].voltspinbox->setValue(voltage);
        settings.endGroup();
    }
    settings.endGroup();

    // Read core pstates
    settings.beginGroup("corePstates");
    QStringList corePstateIndices = settings.childGroups();
    for (int i=0; i<corePstateIndices.size(); i++) {
        settings.beginGroup(corePstateIndices[i]);
        int frequency = settings.value("frequency").toInt();
        int voltage = settings.value("voltage").toInt();
        corePstates[corePstateIndices[i].toInt()].freqspinbox->setValue(frequency);
        corePstates[corePstateIndices[i].toInt()].voltspinbox->setValue(voltage);
        settings.endGroup();
    }
    settings.endGroup();

    QWidget *buttonwidget = new QWidget;
    QVBoxLayout *buttonlo = new QVBoxLayout;
    // Add an apply button
    QPushButton *applyButton = new QPushButton;
    connect(applyButton, SIGNAL(clicked()), SLOT(applyValues()));
    applyButton->setText("Apply values");
    buttonlo->addWidget(applyButton);
    // Add a reset button
    QPushButton *resetButton = new QPushButton;
    connect(resetButton, SIGNAL(clicked()), SLOT(resetPstates()));
    resetButton->setText("Reset to defaults");
    buttonlo->addWidget(resetButton);
    buttonwidget->setLayout(buttonlo);
    llo->addWidget(buttonwidget);

    QStatusBar *statusbar = new QStatusBar;
    statusBar = statusbar;
    QWidget *barwdg = new QWidget;
    QVBoxLayout *barlo = new QVBoxLayout;
    barlo->addWidget(statusbar);
    barwdg->setLayout(barlo);

    lower->setLayout(llo);
    upper->setLayout(ulo);

    QVBoxLayout *mainlo = new QVBoxLayout;
    mainlo->addWidget(upper);
    mainlo->addWidget(lower);
    mainlo->addWidget(barwdg);
    ui->centralWidget->setLayout(mainlo);

    //statusbar->showMessage("test");
}
bool amdPstateEditor::applyValues()
{
    qDebug("Applying values");
    QProcess proc;
    QString volt;
    QString freq;
    QString cmd = "pkexec /bin/sh -c \"";
    // Vector for saving what got applied
    QVector <int> changedMemPstates, changedCorePstates;
    // Apply core pstates
    for (int i=0; i<corePstates.size(); i++) {
        if ((corePstates[i].freqspinbox->value() != types->GPUList[gpuidx].coreclocks[i]) || (corePstates[i].voltspinbox->value() != types->GPUList[gpuidx].corevolts[i])) {
            changedCorePstates.append(i);
            volt = QString::number(corePstates[i].voltspinbox->value());
            freq = QString::number(corePstates[i].freqspinbox->value());
            cmd.append("echo 's "+ QString::number(i) + " "+ freq +" "+ volt +"' "+"> /sys/class/drm/card"+QString::number(types->GPUList[gpuidx].fsindex)+"/device/pp_od_clk_voltage & ");
            qDebug() << cmd;
        }
    }
    // Apply memory pstates
    for (int i=0; i<memPstates.size(); i++) {
        if ((memPstates[i].freqspinbox->value() != types->GPUList[gpuidx].memclocks[i]) || (memPstates[i].voltspinbox->value() != types->GPUList[gpuidx].memvolts[i])) {
            changedMemPstates.append(i);
            volt = QString::number(memPstates[i].voltspinbox->value());
            freq = QString::number(memPstates[i].freqspinbox->value());
            cmd.append("echo 'm "+ QString::number(i) + " "+ freq +" "+ volt +"' "+"> /sys/class/drm/card"+QString::number(types->GPUList[gpuidx].fsindex)+"/device/pp_od_clk_voltage & ");
            qDebug() << cmd;
        }
    }
    if (!changedMemPstates.isEmpty() || !changedCorePstates.isEmpty()) {
        cmd.append("echo 'c' > /sys/class/drm/card" + QString::number(types->GPUList[gpuidx].fsindex) + "/device/pp_od_clk_voltage\"");
        proc.start(cmd);
        proc.waitForFinished(-1);
        if (proc.exitCode() != 0) {
            statusBar->showMessage("Failed to apply changes.");
            return false;
        }
    }
    // Save the values if it was successful
    QSettings settings("tuxclocker");
    QString currentProfile = settings.value("currentProfile").toString();
    settings.beginGroup(currentProfile);
    settings.beginGroup(types->GPUList[gpuidx].pci_id);

    settings.beginWriteArray("memPstates");
    for (int i=0; i<changedMemPstates.size(); i++) {
        settings.setArrayIndex(changedMemPstates[i]);
        types->GPUList[gpuidx].memclocks[changedMemPstates[i]] = memPstates[changedMemPstates[i]].freqspinbox->value();
        types->GPUList[gpuidx].memvolts[changedMemPstates[i]] = memPstates[changedMemPstates[i]].voltspinbox->value();
        settings.setValue("voltage", memPstates[changedMemPstates[i]].voltspinbox->value());
        settings.setValue("frequency", memPstates[changedMemPstates[i]].freqspinbox->value());
    }
    settings.endArray();

    settings.beginWriteArray("corePstates");
    for (int i=0; i<changedCorePstates.size(); i++) {
        settings.setArrayIndex(changedCorePstates[i]);
        types->GPUList[gpuidx].coreclocks[changedCorePstates[i]] = corePstates[changedCorePstates[i]].freqspinbox->value();
        types->GPUList[gpuidx].corevolts[changedCorePstates[i]] = corePstates[changedCorePstates[i]].voltspinbox->value();
        settings.setValue("voltage", corePstates[changedCorePstates[i]].voltspinbox->value());
        settings.setValue("frequency", corePstates[changedCorePstates[i]].freqspinbox->value());
    }
    settings.endArray();

    statusBar->showMessage("Changes applied.");
    return true;
}
bool amdPstateEditor::resetPstates()
{
    bool ret = false;
    QProcess proc;
    proc.start("pkexec /bin/sh -c \"echo 'r' > /sys/class/drm/card" + QString::number(types->GPUList[gpuidx].fsindex)+"/device/pp_od_clk_voltage\"");
    proc.waitForFinished(-1);
    if (proc.exitCode() == 0) {
        ret = true;
    }
    return ret;
}
