#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileInfo>
#include "editprofile.h"
#include <QProcess>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString settingsDir = "C:/Users/Gnometech/Documents/rojekti/";
    QString currentProfile = "profile";
    QString nvFanQ = "nvidia-settings -q GPUCurrentFanSpeed -t";
    QString nvVoltQ = "nvidia-settings -q GPUCurrentCoreVoltage -t";
    QString nvVoltOfsQ = "nvidia-settings -q GPUOverVoltageOffset -t";
    QString nvCoreClkOfsQ = "nvidia-settings -q GPUGraphicsClockOffset -t";
    QString nvCurrentLimitsQ = "nvidia-settings -q GPUPerfModes -t";

    int voltInt;
    int voltOfsInt;
    int coreFreqOfsInt;

private slots:

    void on_actionEdit_current_profile_triggered(bool checked);
    void on_pushButton_clicked();
    void checkForProfiles();
    void on_profileComboBox_activated(const QString &arg1);
    void queryGPUSettings();
    void getGPUInfo();
    void on_frequencySlider_valueChanged(int value);
    void on_frequencySpinBox_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    bool noProfiles;
};

#endif // MAINWINDOW_H
