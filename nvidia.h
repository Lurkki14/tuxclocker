#ifndef NVIDIA_H
#define NVIDIA_H

#include <QObject>
#include <QDebug>
#include <QtX11Extras/QX11Info>
#include <QProcess>
#include "/opt/cuda/include/nvml.h"

class nvidia : public QObject
{
    Q_OBJECT
public:
    explicit nvidia(QObject *parent = nullptr);

    struct GPU
    {
        bool status;
        int index;
        char *name;
        char *uuid;
        char *utils;
        bool overVoltAvailable = false;
        bool overClockAvailable = false;
        bool memOverClockAvailable = false;
        bool voltageReadable = false;
        bool coreClkReadable = false;
        bool memClkReadable = false;
        bool manualFanCtrl = false;
        int maxVoltageOffset;
        int minVoltageOffset;
        int minCoreClkOffset;
        int maxCoreClkOffset;
        int minMemClkOffset;
        int maxMemClkOffset;

        int coreFreq;
        int memFreq;
        int temp;
        int voltage;
        int fanSpeed;
        uint powerDraw;
        uint coreUtil;
        uint memUtil;
        uint minPowerLim;
        uint maxPowerLim;
        uint powerLim;

        int totalVRAM;
        int usedVRAM;
    };
    QVector <GPU> GPUList;
    int gpuCount = 0;
private:
    Display *dpy;

    nvmlDevice_t *device;
signals:

public slots:
    bool setupXNVCtrl();
    bool setupNVML(int GPUIndex);
    void queryGPUCount();
    void queryGPUNames();
    void queryGPUUIDs();
    void queryGPUFeatures();
    void queryGPUVoltage(int GPUIndex);
    void queryGPUTemp(int GPUIndex);
    void queryGPUFrequencies(int GPUIndex);
    void queryGPUFanSpeed(int GPUIndex);
    void queryGPUUsedVRAM(int GPUIndex);

    void queryGPUUtils(int GPUIndex);
    void queryGPUPowerDraw(int GPUIndex);
    void queryGPUPowerLimit(int GPUIndex);
    void queryGPUPowerLimitLimits(int GPUIndex);

    bool assignGPUFanSpeed(int GPUIndex, int targetValue);
    bool assignGPUFanCtlMode(int GPUIndex, int targetValue);
    bool assignGPUFreqOffset(int GPUIndex, int targetValue);
    bool assignGPUMemClockOffset(int GPUIndex, int targetValue);
    bool assignGPUVoltageOffset(int GPUIndex, int targetValue);
    // NVML functions know the GPU index already based on the dev object passed in setupNVML()
    bool assignGPUPowerLimit(uint targetValue);
private slots:
};

#endif // NVIDIA_H
