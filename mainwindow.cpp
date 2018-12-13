#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editprofile.h"
#include "ui_editprofile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    checkForProfiles();

    ui->frequencySlider->setRange(-100, 1000);
    ui->frequencySpinBox->setRange(-100, 1000);
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
    queryGPUSettings();
}

void MainWindow::checkForProfiles()
{
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
void MainWindow::getGPUInfo()
{

}
void MainWindow::queryGPUSettings()
{
    QString volt;
    QProcess process;
    process.start(nvVoltQ);
    process.waitForFinished(-1);
    volt = process.readLine();
    volt.chop(1);
    voltInt = volt.toInt()/1000;
    qDebug() << voltInt;

    QString voltOfs;
    process.start(nvVoltOfsQ);
    process.waitForFinished(-1);
    voltOfs = process.readLine();
    voltOfs.chop(1);
    voltOfsInt = voltOfs.toInt();
    qDebug() << voltOfsInt;

    QString coreFreqOfs;
    process.start(nvCoreClkOfsQ);
    process.waitForFinished(-1);
    coreFreqOfs = process.readLine();
    coreFreqOfs.chop(1);
    coreFreqOfsInt = coreFreqOfs.toInt();
    qDebug() << coreFreqOfsInt;

    QString currentLimits;
    QString nvclockmax = "nvclockmax";
    QRegExp exp;
    process.start(nvCurrentLimitsQ);
    process.waitForFinished(-1);
    currentLimits = process.readAll();
    currentLimits.remove(0, (currentLimits.lastIndexOf("perf=3")));
    //currentLimits.remove(0, (currentLimits.lastIndexOf("nvclockmax")));
    exp.setPattern(nvclockmax);
    //exp.setPatternSyntax(QRegExp::)
    qDebug() << currentLimits;

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
