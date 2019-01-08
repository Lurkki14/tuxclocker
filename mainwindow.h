#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "editprofile.h"
#include "monitor.h"
#include <QProcess>
# include <QList>
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
    QString nvTempQ = "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits";

    QString nvCoreClkSet = "nvidia-settings -a GPUGraphicsClockOffset[3]=";
    QString nvMemClkSet = "nvidia-settings -a GPUMemoryTransferRateOffset[3]=";
    QString nvPowerLimSet = "nvidia-smi -pl ";
    QString nvFanSpeedSet = "nvidia-settings -a GPUTargetFanSpeed=";
    QString nvVoltageSet = "nvidia-settings -a GPUOverVoltageOffset=";

    QString nvFanCtlStateSet = "nvidia-settings -a GPUFanControlState=";

    QString nvFanCtlStateQ = "nvidia-settings -q GPUFanControlState -t";

    QString grepStringToInt = " | egrep -o '[0-9]{0,100}'\"";

    QString queryForNvidiaProp = "/bin/sh -c \"lspci -vnn | grep -c 'Kernel driver in use: nvidia'\"";
    QString queryGPUName = "/bin/sh -c \"nvidia-smi --query-gpu=gpu_name --format=csv | grep '[0-9]'\"";

    QString errorText = "Failed to apply these settings: ";

    QString gpuDriver;
    QVector <int> xCurvePoints, yCurvePoints;

    int voltInt;
    int voltOfsInt;
    int coreFreqOfsInt;

    int maxPowerLimInt;
    int minPowerLimInt;
    int curPowerLimInt;

    int minCoreClkOfsInt=0;
    int maxCoreClkOfsInt=0;
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
    int fanControlMode;

    int defCoreClk;
    int defMemClk;
    int defVolt;

    int latestClkOfs;
    int latestPowerLim;
    int latestMemClkOfs;
    int latestVoltOfs;

    bool isRoot = false;
    bool manualFanCtl;
public slots:

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
    void enableFanUpdater();
    void setupMonitorTab();
    void updateMonitor();
    void saveProfileSettings();
    void loadProfileSettings();
    void checkForProfiles();
    void on_fanModeComboBox_currentIndexChanged(int index);
    void tabHandler(int index);
    void setupGraphMonitorTab();
    void plotHovered(QMouseEvent *event);
private:
    Ui::MainWindow *ui;
    bool noProfiles = true;
    QVector <int> compXPoints, compYPoints;

    QTimer *resettimer = new QTimer(this);
    QTimer *fanUpdateTimer = new QTimer(this);
    QTimer *statusLabelResetTimer = new QTimer(this);
    QTimer *fanUpdaterDisablerTimer = new QTimer(this);
    QTimer *monitorUpdater = new QTimer(this);
    QTimer *plotHoverUpdater = new QTimer(this);

    QTreeWidgetItem *gputemp = new QTreeWidgetItem;
    QTreeWidgetItem *powerdraw = new QTreeWidgetItem;
    QTreeWidgetItem *voltage = new QTreeWidgetItem;
    QTreeWidgetItem *coreclock = new QTreeWidgetItem;
    QTreeWidgetItem *memclock = new QTreeWidgetItem;
    QTreeWidgetItem *coreutil = new QTreeWidgetItem;
    QTreeWidgetItem *memutil = new QTreeWidgetItem;
    QTreeWidgetItem *fanspeed = new QTreeWidgetItem;
    QTreeWidgetItem *memusage = new QTreeWidgetItem;
    QTreeWidgetItem *curmaxclk = new QTreeWidgetItem;
    QTreeWidgetItem *curmaxmemclk = new QTreeWidgetItem;

    // Widgets for the graph monitor
    QWidget *plotWidget = new QWidget;
    QScrollArea *plotScrollArea = new QScrollArea;
    QVBoxLayout *lo = new QVBoxLayout;

    QVBoxLayout *plotLayout = new QVBoxLayout;

    QVBoxLayout *tempLayout = new QVBoxLayout;
    QVBoxLayout *powerDrawLayout = new QVBoxLayout;
    QVBoxLayout *coreClkLayout = new QVBoxLayout;
    QVBoxLayout *memClkLayout = new QVBoxLayout;
    QVBoxLayout *coreUtilLayout = new QVBoxLayout;
    QVBoxLayout *memUtilLayout = new QVBoxLayout;
    QVBoxLayout *voltageLayout = new QVBoxLayout;

    QCustomPlot *tempPlot = new QCustomPlot;
    QCustomPlot *powerDrawPlot = new QCustomPlot;
    QCustomPlot *coreClkPlot = new QCustomPlot;
    QCustomPlot *memClkPlot = new QCustomPlot;
    QCustomPlot *coreUtilPlot = new QCustomPlot;
    QCustomPlot *memUtilPlot = new QCustomPlot;
    QCustomPlot *voltagePlot = new QCustomPlot;

    QWidget *tempWidget = new QWidget;
    QWidget *powerDrawWidget = new QWidget;
    QWidget *coreClkWidget = new QWidget;
    QWidget *memClkWidget = new QWidget;
    QWidget *coreUtilWidget = new QWidget;
    QWidget *memUtilWidget = new QWidget;
    QWidget *voltageWidget = new QWidget;

    QVector <double> qv_time, qv_temp, qv_powerDraw, qv_coreClk, qv_memClk, qv_coreUtil, qv_memUtil, qv_voltage;
    double tempnum;
    double powernum = 0;
    struct plotCmds
    {
        QVector <double> vector;
        double valueq;
        QCustomPlot *plot;
        QVBoxLayout *layout;
        QWidget *widget;
    };
    int counter = 0;

    plotCmds powerdrawplot;
    plotCmds tempplot;
    plotCmds coreclkplot;
    plotCmds memclkplot;
    plotCmds coreutilplot;
    plotCmds memutilplot;
    plotCmds voltageplot;
    QVector <plotCmds> plotCmdsList;
};
#endif // MAINWINDOW_H
