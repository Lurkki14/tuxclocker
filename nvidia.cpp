#include "nvidia.h"
#include <X11/Xlib.h>

#include "NVCtrl/NVCtrlLib.h"

nvidia::nvidia(QObject *parent) : QObject(parent)
{

}
bool nvidia::setupXNVCtrl()
{
    // Open the x-server connection and check if the extension exists
    dpy = XOpenDisplay(nullptr);
    Bool ret;
    int *event_basep = nullptr;
    int *error_basep = nullptr;

    ret = XNVCTRLQueryExtension(dpy,
                                event_basep,
                                error_basep);
    qDebug() << ret;

    queryGPUCount();
    queryGPUNames();
    queryGPUUIDs();
    queryGPUFeatures();
    return ret;
}
void nvidia::queryGPUCount()
{
    Bool ret;
    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU, &gpuCount);
    if (!ret) {
        qDebug() << "Failed to query amount of GPUs";
    }
    // Create an appropriate number of GPU objects
    for (int i=0; i<gpuCount; i++) {
        GPU gpu;
        GPUList.append(gpu);
    }
    qDebug() << gpuCount;
}
void nvidia::queryGPUNames()
{
    Bool ret;
    for (int i=0; i<gpuCount; i++) {
        ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                                NV_CTRL_TARGET_TYPE_GPU,
                                                i,
                                                0,
                                                NV_CTRL_STRING_PRODUCT_NAME,
                                                &GPUList[i].name);
        if (!ret) {
            qDebug() << "Failed to query GPU Name";
        }
        qDebug() << GPUList[i].name;
    }
}
void nvidia::queryGPUUIDs()
{
    Bool ret;
    for (int i=0; i<gpuCount; i++) {
        ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                                NV_CTRL_TARGET_TYPE_GPU,
                                                i,
                                                0,
                                                NV_CTRL_STRING_GPU_UUID,
                                                &GPUList[i].uuid);
        if (!ret) {
            qDebug() << "Failed to query GPU UUID";
        }
        qDebug() << GPUList[i].uuid;
    }
}
void nvidia::queryGPUFeatures()
{
    // Query static features related to the GPU such as maximum offsets here
    Bool ret;
    NVCTRLAttributeValidValuesRec values;
    for (int i=0; i<gpuCount; i++) {
        ret = XNVCTRLQueryValidTargetAttributeValues(dpy,
                                                     NV_CTRL_TARGET_TYPE_GPU,
                                                     i,
                                                     0,
                                                     NV_CTRL_GPU_OVER_VOLTAGE_OFFSET,
                                                     &values);
        qDebug() << values.u.range.max << values.permissions << "value";

        GPUList[i].maxVoltageOffset = values.u.range.max;
    }

    queryGPUVoltage(0);
    queryGPUTemp(0);
    queryGPUFrequency(0);
    queryGPUFanSpeed(0);
    //assignFanSpeed(0, 60);
}
void nvidia::queryGPUVoltage(int GPUIndex)
{
    Bool ret;
    ret = XNVCTRLQueryTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      GPUIndex,
                                      0,
                                      NV_CTRL_GPU_CURRENT_CORE_VOLTAGE,
                                      &GPUList[GPUIndex].voltage);
    if (!ret) {
        qDebug() << "Couldn't query voltage for GPU";
    }
    qDebug() << GPUList[GPUIndex].voltage;
}
void nvidia::queryGPUTemp(int GPUIndex)
{
    Bool ret;
    ret = XNVCTRLQueryTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                                      GPUIndex,
                                      0,
                                      NV_CTRL_THERMAL_SENSOR_READING,
                                      &GPUList[GPUIndex].temp);
    qDebug() << GPUList[GPUIndex].temp;
}
void nvidia::queryGPUFrequency(int GPUIndex)
{
    Bool ret;
    int packedInt = 0;
    int mem = 0;
    int core = 0;
    ret = XNVCTRLQueryTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      GPUIndex,
                                      0,
                                      NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
                                      &packedInt);
    // The function returns a 32-bit packed integer, GPU Clock is the upper 16 and Memory Clock lower 16
    mem = (packedInt) & 0xFFFF;
    core = (packedInt & (0xFFFF << (32 - 16))) >> (32 - 16);
    qDebug() << GPUList[GPUIndex].coreFreq << "freq" << core << mem;
    GPUList[GPUIndex].coreFreq = core;
    GPUList[GPUIndex].memFreq = mem;
}
void nvidia::queryGPUFanSpeed(int GPUIndex)
{
    Bool ret;
    ret = XNVCTRLQueryTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_COOLER,
                                      GPUIndex,
                                      0,
                                      NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL,
                                      &GPUList[GPUIndex].fanSpeed);

    qDebug() << GPUList[GPUIndex].fanSpeed;
}
bool nvidia::assignFanSpeed(int GPUIndex, int targetValue)
{
    Bool ret;
    ret = XNVCTRLSetTargetAttributeAndGetStatus(dpy,
                                                NV_CTRL_TARGET_TYPE_COOLER,
                                                GPUIndex,
                                                0,
                                                NV_CTRL_THERMAL_COOLER_LEVEL,
                                                targetValue);
    return ret;
}
/*bool nvidia::setup()
{
    dpy = XOpenDisplay(nullptr);
    int *temp;
    XNVCTRLQueryTargetAttribute(dpy,
                                NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                                0,
                                0,
                                NV_CTRL_THERMAL_SENSOR_READING,
                                temp);
    qDebug() << temp;
    return true;
}*/
