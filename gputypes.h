#ifndef GPUTYPES_H
#define GPUTYPES_H

#include <QObject>
#include <QVector>
#include <QDebug>
#include <QDir>
#include <QtX11Extras/QX11Info>
#ifdef NVIDIA
#include "nvml.h"
#endif

#ifdef AMD
#include <sys/ioctl.h>
#include <fcntl.h>
#include <libdrm/amdgpu_drm.h>
#include <libdrm/amdgpu.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#endif

class gputypes : public QObject
{
    Q_OBJECT
public:
    gputypes();
    enum Type{NV, AMDGPU};
    struct GPU
    {
        int gputype;
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

        // AMD only:
        // GPU index in the filesystem eg. card0
        int fsindex;
        // name of the folder in /sys/class/drm/card(n)/device/hwmon
        QString hwmonpath;
    };
    QVector <GPU> GPUList;

    int gpuCount = 0;
#ifdef NVIDIA
    Display *dpy;
    nvmlDevice_t *device;
#endif

#ifdef AMD
    amdgpu_device_handle *dev;
#endif

    virtual bool setupGPU() = 0;
    virtual bool setupGPUSecondary(int GPUIndex) = 0;
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
    virtual bool assignGPUFanCtlMode(int GPUIndex, bool targetStatus) = 0;
    virtual bool assignGPUFreqOffset(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUMemClockOffset(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUVoltageOffset(int GPUIndex, int targetValue) = 0;
    virtual bool assignGPUPowerLimit(uint targetValue) = 0;
protected:
    ~gputypes();

private:
};
#ifdef NVIDIA
class nvidia : public gputypes
{
    Q_OBJECT
public:
    nvidia();
signals:
public slots:
    bool setupGPU();
    bool setupGPUSecondary(int GPUIndex);
    void queryGPUCount();
    void queryGPUNames();
    void queryGPUUIDs();
    void queryGPUFeatures();
    void queryGPUVoltage(int GPUIndex);
    void queryGPUTemp(int GPUIndex);
    void queryGPUFrequencies(int GPUIndex);
    void queryGPUFanSpeed(int GPUIndex);
    void queryGPUUsedVRAM(int GPUIndex);
    void queryGPUFreqOffset(int GPUIndex);
    void queryGPUMemClkOffset(int GPUIndex);
    void queryGPUVoltageOffset(int GPUIndex);

    void queryGPUUtils(int GPUIndex);
    void queryGPUPowerDraw(int GPUIndex);
    void queryGPUPowerLimit(int GPUIndex);
    void queryGPUPowerLimitLimits(int GPUIndex);
    void queryGPUCurrentMaxClocks(int GPUIndex);
    void queryGPUPowerLimitAvailability(int GPUIndex);

    bool assignGPUFanSpeed(int GPUIndex, int targetValue);
    bool assignGPUFanCtlMode(int GPUIndex, bool manual);
    bool assignGPUFreqOffset(int GPUIndex, int targetValue);
    bool assignGPUMemClockOffset(int GPUIndex, int targetValue);
    bool assignGPUVoltageOffset(int GPUIndex, int targetValue);
    // NVML functions know the GPU index already based on the dev object passed in setupNVML()
    bool assignGPUPowerLimit(uint targetValue);
private slots:
};
#endif

#ifdef AMD
class amd : public gputypes
{
    Q_OBJECT
public:
    amd();
signals:
public slots:
    bool setupGPU();
    bool setupGPUSecondary(int GPUIndex);
    void queryGPUCount();
    void queryGPUNames();
    void queryGPUUIDs();
    void queryGPUFeatures();
    void queryGPUVoltage(int GPUIndex);
    void queryGPUTemp(int GPUIndex);
    void queryGPUFrequencies(int GPUIndex);
    void queryGPUFanSpeed(int GPUIndex);
    void queryGPUUsedVRAM(int GPUIndex);
    void queryGPUFreqOffset(int GPUIndex);
    void queryGPUMemClkOffset(int GPUIndex);
    void queryGPUVoltageOffset(int GPUIndex);

    void queryGPUUtils(int GPUIndex);
    void queryGPUPowerDraw(int GPUIndex);
    void queryGPUPowerLimit(int GPUIndex);
    void queryGPUPowerLimitLimits(int GPUIndex);
    void queryGPUCurrentMaxClocks(int GPUIndex);
    void queryGPUPowerLimitAvailability(int GPUIndex);

    bool assignGPUFanSpeed(int GPUIndex, int targetValue);
    bool assignGPUFanCtlMode(int GPUIndex, bool manual);
    bool assignGPUFreqOffset(int GPUIndex, int targetValue);
    bool assignGPUMemClockOffset(int GPUIndex, int targetValue);
    bool assignGPUVoltageOffset(int GPUIndex, int targetValue);
    // NVML functions know the GPU index already based on the dev object passed in setupNVML()
    bool assignGPUPowerLimit(uint targetValue);
private slots:
};
#endif

#endif // GPUTYPES_H
