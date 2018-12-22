#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editprofile.h"
#include "ui_editprofile.h"
#include "newprofile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    checkForProfiles();
    loadProfileSettings();
    queryGPUSettings();
    enableFanControl();

    ui->frequencySlider->setRange(defCoreClk + minCoreClkOfsInt, defCoreClk + maxCoreClkOfsInt);
    ui->frequencySpinBox->setRange(defCoreClk + minCoreClkOfsInt, defCoreClk + maxCoreClkOfsInt);
    ui->frequencySlider->setValue(defCoreClk + coreFreqOfsInt);
    ui->frequencySpinBox->setValue(defCoreClk + coreFreqOfsInt);

    ui->powerLimSlider->setRange(minPowerLimInt, maxPowerLimInt);
    ui->powerLimSpinBox->setRange(minPowerLimInt, maxPowerLimInt);
    ui->powerLimSlider->setValue(curPowerLimInt);
    ui->powerLimSpinBox->setValue(curPowerLimInt);

    ui->memClkSlider->setRange(defMemClk + minMemClkOfsInt, defMemClk + maxMemClkOfsInt);
    ui->memClkSpinBox->setRange(defMemClk + minMemClkOfsInt, defMemClk + maxMemClkOfsInt);
    ui->memClkSlider->setValue(defMemClk + memClkOfsInt);
    ui->memClkSpinBox->setValue(defMemClk + memClkOfsInt);

    ui->voltageSlider->setRange(voltInt + minVoltOfsInt, voltInt + maxVoltOfsInt);
    ui->voltageSpinBox->setRange(voltInt + minVoltOfsInt, voltInt + maxVoltOfsInt);
    ui->voltageSlider->setValue(voltInt + voltOfsInt);
    ui->voltageSpinBox->setValue(voltInt + voltOfsInt);

    ui->fanSlider->setValue(fanSpeed);
    ui->fanSpinBox->setValue(fanSpeed);
    ui->fanSlider->setRange(0, 100);
    ui->fanSpinBox->setRange(0, 100);

    QTimer *fanUpdateTimer = new QTimer(this);
    connect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(fanSpeedUpdater()));
    //connect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(tempUpdater()));
    fanUpdateTimer->start(2000);

    connect(ui->frequencySpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
    connect(ui->powerLimSpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
    connect(ui->memClkSpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
    connect(ui->voltageSpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionEdit_current_profile_triggered(bool)
{
    //editProfile editprof;
    //editprof.setModal(false);
    //editprof.exec();


    editProfile *editprof = new editProfile(this);
    editprof->show();
}

void MainWindow::on_pushButton_clicked()
{
    qDebug() << currentProfile;
    //queryGPUSettings();
    //loadProfileSettings();
   // checkForProfiles();
    //getGPUDriver();
    checkForRoot();
}

void MainWindow::checkForRoot()
{
    QProcess process;
    process.start("/bin/sh -c \"echo $EUID\"");
    process.waitForFinished();
    QString EUID = process.readLine();
    if (EUID.toInt() == 0) {
        isRoot = true;
    }
    qDebug() << isRoot;
}
void MainWindow::checkForProfiles()
{
    qDebug() << "chkproffunc";
    // If there are no profiles, create one, then list all the entries whose isProfile is true in the profile selection combo box
    QSettings settings("nvfancurve");
    QStringList groups = settings.childGroups();
    QString isProfile = "/isProfile";

    for (int i=0; i<groups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = groups[i];
        QString query = group.append(isProfile);
        if (settings.value(query).toBool()) {
            noProfiles = false;
            break;
        }
    }
    if (noProfiles) {
        settings.setValue("New Profile/isProfile", true);
        settings.setValue("General/currentProfile", "New Profile");
        currentProfile = "New Profile";
    }
    // Redefine child groups so it contains the newly created profile if it was made
    QStringList newgroups = settings.childGroups();
    for (int i=0; i<newgroups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = newgroups[i];
        QString query = group.append(isProfile);
        if (settings.value(query).toBool()) {
            ui->profileComboBox->addItem(newgroups[i]);
        }
    }
}

void MainWindow::on_profileComboBox_activated(const QString &arg1)
{
    // Change currentProfile to combobox selection
    currentProfile = arg1;
}

void MainWindow::getGPUDriver()
{
    QProcess process;
    process.start(queryForNvidiaProp);
    process.waitForFinished(-1);
    if (process.readAllStandardOutput().toInt() >= 1) {
        gpuDriver = "nvidia";
    }
}
void MainWindow::enableFanControl()
{
    QProcess process;
    process.start("nvidia-settings -a GPUFanControlState=1");
    process.waitForFinished(-1);
}
void MainWindow::getGPUInfo()
{

}

void MainWindow::fanSpeedUpdater()
{
    QProcess process;
    process.start(nvFanQ);
    process.waitForFinished(-1);
    fanSpeed = process.readLine().toInt();
    ui->fanSlider->setValue(fanSpeed);
    ui->fanSpinBox->setValue(fanSpeed);

}
void MainWindow::tempUpdater()
{
    QProcess process;
    process.start(nvTempQ);
    process.waitForFinished(-1);
    temp = process.readLine().toInt();
    if (xCurvePoints.size() != 0) {
        generateFanPoint();
    }
}
void MainWindow::resetTimer()
{
    // If a value has been changed this timer will start. When the apply button has been pressed, this gets cancelled
    connect(resettimer, SIGNAL(timeout()), SLOT(resetChanges()));
    resettimer->stop();
    resettimer->setSingleShot(true);
    resettimer->start(10000);
}
void MainWindow::resetChanges()
{
    // If the settings haven't been applied in 10 seconds, reset all values to their latest values
    ui->frequencySlider->setValue(defCoreClk + latestClkOfs);
    ui->frequencySpinBox->setValue(defCoreClk + latestClkOfs);

    ui->powerLimSlider->setValue(latestPowerLim);
    ui->powerLimSpinBox->setValue(latestPowerLim);

    ui->voltageSlider->setValue(voltInt + latestVoltOfs);
    ui->voltageSpinBox->setValue(voltInt + latestVoltOfs);

    ui->memClkSlider->setValue(curMaxMemClkInt);
    ui->memClkSpinBox->setValue(curMaxMemClkInt);

    ui->memClkSlider->setValue(defMemClk + latestMemClkOfs);
    ui->memClkSpinBox->setValue(defMemClk + latestMemClkOfs);
    qDebug() << "timer";
}
void MainWindow::queryGPUSettings()
{
    QProcess process;
    process.start(nvVoltQ);
    process.waitForFinished(-1);
    voltInt = process.readLine().toInt()/1000;

    process.start(nvVoltOfsQ);
    process.waitForFinished(-1);
    voltOfsInt = process.readLine().toInt()/1000;
    latestVoltOfs = voltOfsInt;

    defVolt = voltInt - voltOfsInt;

    process.start(nvVoltOfsLimQ);
    process.waitForFinished(-1);
    for (int i=0; i<process.size(); i++) {
        if (process.readLine().toInt()/1000 > maxVoltOfsInt) {
            qDebug() << process.readLine();
            maxVoltOfsInt = process.readLine().toInt()/1000;
        }
    }
    qDebug() << "maxvolt on" << maxVoltOfsInt;

    QString coreFreqOfs;
    process.start(nvCoreClkOfsQ);
    process.waitForFinished(-1);
    coreFreqOfs = process.readLine();
    coreFreqOfs.chop(1);
    coreFreqOfsInt = coreFreqOfs.toInt();
    latestClkOfs = coreFreqOfsInt;

    process.start(nvCurMaxClkQ);
    process.waitForFinished(-1);
    curMaxClkInt = process.readLine().toInt();
    qDebug() << curMaxClkInt;

    process.start(nvMaxPowerLimQ);
    process.waitForFinished(-1);
    maxPowerLimInt = process.readLine().toInt();

    process.start(nvMinPowerLimQ);
    process.waitForFinished(-1);
    minPowerLimInt = process.readLine().toInt();

    process.start(nvCurPowerLimQ);
    process.waitForFinished(-1);
    curPowerLimInt = process.readLine().toInt();
    latestPowerLim = curPowerLimInt;

    process.start(nvClockLimQ);
    process.waitForFinished(-1);
    for (int i=0; i<2; i++) {
        QString line = process.readLine();
        if (line.contains("-")) {
            minCoreClkOfsInt = line.toInt();
        } else {
            maxCoreClkOfsInt = line.toInt();
        }
    }
    qDebug() << minCoreClkOfsInt << maxCoreClkOfsInt;

    // This gets the transfer rate, the clock speed is rate/2
    process.start(nvMemClkLimQ);
    process.waitForFinished(-1);
    for (int i=0; i<2; i++) {
        QString line = process.readLine();
        if (line.contains("-")) {
            minMemClkOfsInt = line.toInt()/2;
        } else {
            maxMemClkOfsInt = line.toInt()/2;
        }
    }

    process.start(nvCurMaxMemClkQ);
    process.waitForFinished(-1);
    curMaxMemClkInt = process.readLine().toInt();

    process.start(nvCurMemClkOfsQ);
    process.waitForFinished(-1);
    memClkOfsInt = process.readLine().toInt()/2;
    latestMemClkOfs = memClkOfsInt;

    // Since the maximum core clock reported is the same on negative offsets as on 0 offset add a check here
    if (0 >= coreFreqOfsInt) {
        defCoreClk = curMaxClkInt;
    } else {
        defCoreClk = curMaxClkInt - coreFreqOfsInt;
    }
    defMemClk = curMaxMemClkInt - memClkOfsInt;
}
void MainWindow::applyGPUSettings()
{
    QProcess process;
    int offsetValue;
    int powerLimit;
    QString input = nvCoreClkSet;
    if (latestClkOfs != ui->frequencySlider->value() - defCoreClk) {
        offsetValue = ui->frequencySlider->value() - defCoreClk;
        QString input = nvCoreClkSet;
        input.append(QString::number(offsetValue));
        qDebug() << input;
        process.start(input);
        process.waitForFinished(-1);
        latestClkOfs = ui->frequencySlider->value() - defCoreClk;
    }

    if (latestMemClkOfs != ui->memClkSlider->value() - defMemClk) {
        offsetValue = ui->memClkSlider->value() - defMemClk;
        input = nvMemClkSet;
        input.append(QString::number(offsetValue*2));
        qDebug() << input;
        process.start(input);
        process.waitForFinished(-1);
        latestMemClkOfs = ui->memClkSlider->value() - defMemClk;
    }

    if (latestPowerLim != ui->powerLimSlider->value()) {
        powerLimit = ui->powerLimSlider->value();
        input = nvPowerLimSet;
        if (!isRoot) {
            input.append(QString::number(powerLimit) +"'\"");
            input.prepend("/bin/sh -c \"kdesu -c ");
            process.start(input);
            process.waitForFinished(-1);
        } else {
            input.append(QString::number(powerLimit));
            process.start(input);
            process.waitForFinished(-1);
            qDebug() << "ran as root";
        }
        latestPowerLim = ui->powerLimSlider->value();
    }

    if (latestVoltOfs != ui->voltageSlider->value() - defVolt) {
        offsetValue = ui->voltageSlider->value() - defVolt;
        input = nvVoltageSet;
        input.append(QString::number(offsetValue*1000));
        qDebug() << input;
        process.start(input);
        process.waitForFinished(-1);
        latestVoltOfs = ui->voltageSlider->value() - defVolt;
    }
    resettimer->stop();
}
void MainWindow::loadProfileSettings()
{
    QSettings settings("nvfancurve");
    currentProfile = settings.value("General/currentProfile").toString();
    settings.beginGroup(currentProfile);
    // Check for existance of the setting so zeroes don't get appended to curve point vectors
    if (settings.contains("xpoints")) {
        QString xPointStr = settings.value("xpoints").toString();
        QString yPointStr = settings.value("ypoints").toString();
        xPointStr.prepend("/bin/sh -c \"echo ");
        xPointStr.append(grepStringToInt);
        QProcess process;
        process.start(xPointStr);
        process.waitForFinished(-1);
        for (int i=0; i<process.size() +1; i++) {
            xCurvePoints.append(process.readLine().toInt());
        }
        yPointStr.prepend("/bin/sh -c \"echo ");
        yPointStr.append(grepStringToInt);
        process.start(yPointStr);
        process.waitForFinished(-1);
        for (int i=0; i<process.size() +1; i++) {
            yCurvePoints.append(process.readLine().toInt());
        }
    }
    qDebug() << xCurvePoints << yCurvePoints;
}

void MainWindow::on_newProfile_closed()
{
    qDebug() << "dialog closed";
    ui->profileComboBox->clear();
    QSettings settings ("nvfancurve");
    QString isProfile = "/isProfile";
    // Refresh the profile combo box
    QStringList newgroups = settings.childGroups();
    for (int i=0; i<newgroups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = newgroups[i];
        QString query = group.append(isProfile);
        if (settings.value(query).toBool()) {
            ui->profileComboBox->addItem(newgroups[i]);
        }
    }
}
void MainWindow::saveProfileSettings()
{
    QSettings settings("nvfancurve");
    settings.beginGroup(currentProfile);

}

void MainWindow::generateFanPoint()
{
    // Calculate the value for fan speed based on temperature
    // First check if the fan speed should be y[0] or y[final]
    int index = 0;
    if (temp <= xCurvePoints[0]) {
        targetFanSpeed = yCurvePoints[0];
    }
    if (temp >= xCurvePoints[xCurvePoints.size()-1]) {
        targetFanSpeed = yCurvePoints[yCurvePoints.size()-1];
    } else {
        // Get the index of the leftmost point of the interpolated interval by comparing it to temperature
        for (int i=0; i<xCurvePoints.size(); i++) {
            if (temp >= xCurvePoints[i] && temp <= xCurvePoints[i+1]) {
                index = i;
                break;
            }
        }
        // Check if the change in x is zero to avoid dividing by it
        if (xCurvePoints[index] - xCurvePoints[index + 1] == 0) {
            targetFanSpeed = yCurvePoints[index+1];
        } else {
            targetFanSpeed = (((yCurvePoints[index + 1] - yCurvePoints[index]) * (temp - xCurvePoints[index])) / (xCurvePoints[index + 1] - xCurvePoints[index])) + yCurvePoints[index];
        }
    }
    qDebug() << targetFanSpeed << index << xCurvePoints << yCurvePoints;
    QProcess process;
    QString input = nvFanSpeedSet;
    input.append(QString::number(targetFanSpeed));
    qDebug() << input;
    process.start(input);
    process.waitForFinished(-1);
}
void MainWindow::on_frequencySlider_valueChanged(int value)
{
    // Sets the input field value to slider value
    QString freqText = QString::number(value);
    ui->frequencySpinBox->setValue(value);
}

void MainWindow::on_frequencySpinBox_valueChanged(int arg1)
{
    ui->frequencySlider->setValue(arg1);
}

void MainWindow::on_newProfile_clicked()
{
    newProfile *newprof = new newProfile(this);
    newprof->setAttribute(Qt::WA_DeleteOnClose);
    connect(newprof, SIGNAL(destroyed(QObject*)), SLOT(on_newProfile_closed()));
    newprof->setModal(true);
    newprof->exec();
}

void MainWindow::on_powerLimSlider_valueChanged(int value)
{
    ui->powerLimSpinBox->setValue(value);
}

void MainWindow::on_powerLimSpinBox_valueChanged(int arg1)
{
    ui->powerLimSlider->setValue(arg1);
}

void MainWindow::on_memClkSlider_valueChanged(int value)
{
    ui->memClkSpinBox->setValue(value);
}

void MainWindow::on_memClkSpinBox_valueChanged(int arg1)
{
    ui->memClkSlider->setValue(arg1);
}

void MainWindow::on_voltageSlider_valueChanged(int value)
{
    ui->voltageSpinBox->setValue(value);
}

void MainWindow::on_voltageSpinBox_valueChanged(int arg1)
{
    ui->voltageSlider->setValue(arg1);
}

void MainWindow::on_fanSlider_valueChanged(int value)
{
    ui->fanSpinBox->setValue(value);
}

void MainWindow::on_fanSpinBox_valueChanged(int arg1)
{
    ui->fanSlider->setValue(arg1);
}

void MainWindow::on_applyButton_clicked()
{
    applyGPUSettings();
}
