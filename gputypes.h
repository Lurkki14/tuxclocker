#ifndef GPUTYPES_H
#define GPUTYPES_H

#include <QObject>
/*#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrlLib.h>
#include "nvml.h"
#include <QVector>*/

class gputypes
{
public:
    gputypes();
    /*struct GPU
    {
        char *name;
        char *uuid;
        bool overVoltAvailable = false;
        bool overClockAvailable = false;
        bool memOverClockAvailable = false;
        bool powerLimitAvailable = false;
        bool voltageReadable = false;
        bool coreClkReadable = false;
        bool memClkReadable = false;
        bool manualFanCtrlAvailable = false;
        int fanControlMode;
        int maxVoltageOffset;
        int minVoltageOffset;
        int minCoreClkOffset;
        int maxCoreClkOffset;
        int minMemClkOffset;
        int maxMemClkOffset;
        uint maxCoreClk;
        uint maxMemClk;

        int voltageOffset;
        int coreClkOffset;
        int memClkOffset;

        int coreFreq;
        int memFreq;
        int temp;
        int voltage;
        int fanSpeed;

        double powerDraw;
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
    Display *dpy;
    nvmlDevice_t *device;*/

    virtual bool setupXNVCtrl() = 0;
    virtual bool setupNVML(int GPUIndex) = 0;
    virtual void queryGPUCount() = 0;
    virtual void queryGPUNames() = 0;
    virtual void queryGPUUIDs() = 0;
    virtual void queryGPUFeatures() = 0;
    virtual void queryGPUVoltage(int GPUIndex) = 0;
    virtual void queryGPUTemp(int GPUIndex) = 0;
    virtual void queryGPUFrequencies(int GPUIndex) = 0;
    virtual void queryGPUFanSpeed(int GPUIndex) = 0;
    virtual void queryGPUUsedVRAM(int GPUIndex) = 0;
    virtual void queryGPUFreqOffset(int GPUIndex) = 0;
    virtual void queryGPUMemClkOffset(int GPUIndex) = 0;
    virtual void queryGPUVoltageOffset(int GPUIndex) = 0;
    virtual void queryGPUUtils(int GPUIndex) = 0;
    virtual void queryGPUPowerDraw(int GPUIndex) = 0;
    virtual void queryGPUPowerLimit(int GPUIndex) = 0;
    virtual void queryGPUPowerLimitLimits(int GPUIndex) = 0;
    virtual void queryGPUCurrentMaxClocks(int GPUIndex) = 0;
    virtual void queryGPUPowerLimitAvailability(int GPUIndex) = 0;

    virtual bool assignGPUFanSpeed(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUFanCtlMode(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUFreqOffset(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUMemClockOffset(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUVoltageOffset(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUPowerLimit(uint targetValue) = 0;
protected:
    ~gputypes();

private:
};

#endif // GPUTYPES_H
