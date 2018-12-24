#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileInfo>
#include "editprofile.h"
#include <QProcess>
//#include "/opt/cuda/include/nvml.h"
//#include <NVCtrl/NVCtrl.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString currentProfile;
    QString nvFanQ = "/bin/sh -c \"nvidia-smi --query-gpu=fan.speed --format=csv | egrep -o '[0-9]{1,4}'\"";
    QString nvVoltQ = "nvidia-settings -q GPUCurrentCoreVoltage -t";
    QString nvVoltOfsQ = "nvidia-settings -q GPUOverVoltageOffset -t";
    QString nvVoltOfsLimQ = "/bin/sh -c \"nvidia-settings -a GPUOverVoltageOffset=99999999 | egrep -o '[0-9]{1,9}'\"";
    QString nvCoreClkOfsQ = "nvidia-settings -q GPUGraphicsClockOffset[3] -t";
    QString nvCurMaxClkQ = "/bin/sh -c \"nvidia-smi --query-supported-clocks=gr --format=csv | egrep -o '[0-9]{2,9}'\"";
    QString nvMaxPowerLimQ = "/bin/sh -c \"nvidia-smi --query-gpu=power.max_limit --format=csv | egrep -o '[0-9]{1,7}'\"";
    QString nvMinPowerLimQ = "/bin/sh -c \"nvidia-smi --query-gpu=power.min_limit --format=csv | egrep -o '[0-9]{1,7}'\"";
    QString nvCurPowerLimQ = "/bin/sh -c \"nvidia-smi --query-gpu=power.limit --format=csv | egrep -o '[0-9]{1,7}'\"";
    QString nvClockLimQ = "/bin/sh -c \"nvidia-settings -a GPUGraphicsClockOffset[3]=999999 | egrep -o '[-0-9]{2,9}'\"";
    QString nvMemClkLimQ = "/bin/sh -c \"nvidia-settings -a GPUMemoryTransferRateOffset[3]=999999 | egrep -o '[-0-9]{2,9}'\"";
    QString nvCurMaxMemClkQ = "/bin/sh -c \"nvidia-smi --query-supported-clocks=mem --format=csv | egrep -o '[0-9]{2,9}'\"";
    QString nvCurMemClkOfsQ = "nvidia-settings -q GPUMemoryTransferRateOffset[3] -t";
    QString nvTempQ = "/bin/sh -c \"nvidia-smi --query-gpu=temperature.gpu --format=csv | egrep -o '[1-9]{1,4}'\"";

    QString nvCoreClkSet = "nvidia-settings -a GPUGraphicsClockOffset[3]=";
    QString nvMemClkSet = "nvidia-settings -a GPUMemoryTransferRateOffset[3]=";
    QString nvPowerLimSet = "'nvidia-smi -pl ";
    QString nvFanSpeedSet = "nvidia-settings -a GPUTargetFanSpeed=";
    QString nvVoltageSet = "nvidia-settings -a GPUOverVoltageOffset=";

    QString nvFanCtlStateSet = "nvidia-settings -a GPUFanControlState=";

    QString nvFanCtlStateQ = "nvidia-settings -q GPUFanControlState -t";

    QString grepStringToInt = " | egrep -o '[0-9]{0,100}'\"";

    QString queryForNvidiaProp = "/bin/sh -c \"lspci -vnn | grep -c 'Kernel driver in use: nvidia'\"";
    QString queryGPUName = "/bin/sh -c \"nvidia-smi --query-gpu=gpu_name --format=csv | grep '[0-9]'\"";

    QString gpuDriver;
    QVector <int> xCurvePoints, yCurvePoints;

    int voltInt;
    int voltOfsInt;
    int coreFreqOfsInt;

    int maxPowerLimInt;
    int minPowerLimInt;
    int curPowerLimInt;

    int minCoreClkOfsInt;
    int maxCoreClkOfsInt;
    int curMaxClkInt;
    int minMemClkOfsInt = 0;
    int maxMemClkOfsInt = 0;
    int minVoltOfsInt = 0;
    int maxVoltOfsInt = 0;
    int curMaxMemClkInt;
    int memClkOfsInt;
    int fanSpeed;
    int temp;
    int targetFanSpeed;

    int defCoreClk;
    int defMemClk;
    int defVolt;

    int latestClkOfs;
    int latestPowerLim;
    int latestMemClkOfs;
    int latestVoltOfs;

    bool isRoot;
    bool manualFanCtl;
public slots:
    void saveProfileSettings();
    void loadProfileSettings();
    void checkForProfiles();
private slots:

    void on_actionEdit_current_profile_triggered(bool checked);
    void on_pushButton_clicked();

    void on_profileComboBox_activated(const QString &arg1);
    void queryGPUSettings();
    void getGPUName();
    void on_frequencySlider_valueChanged(int value);
    void on_frequencySpinBox_valueChanged(int arg1);

    void on_newProfile_clicked();

    void on_powerLimSlider_valueChanged(int value);

    void on_powerLimSpinBox_valueChanged(int arg1);

    void on_newProfile_closed();
    void on_memClkSlider_valueChanged(int value);

    void on_memClkSpinBox_valueChanged(int arg1);

    void on_voltageSlider_valueChanged(int value);

    void on_voltageSpinBox_valueChanged(int arg1);

    void fanSpeedUpdater();
    void applyGPUSettings();
    void on_fanSlider_valueChanged(int value);

    void on_fanSpinBox_valueChanged(int arg1);

    void on_applyButton_clicked();

    void getGPUDriver();
    void generateFanPoint();
    void checkForRoot();
    void tempUpdater();
    void resetChanges();
    void resetTimer();

    void queryDriverSettings();
    void on_editFanCurveButton_pressed();

    void on_editProfile_closed();
    void applyFanMode();
    void resetStatusLabel();
private:
    Ui::MainWindow *ui;
    bool noProfiles = true;
    QVector <int> compXPoints, compYPoints;

    QTimer *resettimer = new QTimer(this);
    QTimer *fanUpdateTimer = new QTimer(this);
    QTimer *statusLabelResetTimer = new QTimer(this);
};
#endif // MAINWINDOW_H
