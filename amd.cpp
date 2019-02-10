#ifdef AMD
#include "gputypes.h"

amd::amd() {}
bool amd::setupGPU()
{
    return true;
}
bool amd::setupGPUSecondary(int GPUIndex){return  true;}
void amd::queryGPUCount(){}
void amd::queryGPUNames(){}
void amd::queryGPUUIDs(){}
void amd::queryGPUFeatures(){}
void amd::queryGPUVoltage(int GPUIndex){}
void amd::queryGPUTemp(int GPUIndex){}
void amd::queryGPUFrequencies(int GPUIndex){}
void amd::queryGPUFanSpeed(int GPUIndex){}
void amd::queryGPUUsedVRAM(int GPUIndex){}
void amd::queryGPUFreqOffset(int GPUIndex){}
void amd::queryGPUMemClkOffset(int GPUIndex){}
void amd::queryGPUVoltageOffset(int GPUIndex){}

void amd::queryGPUUtils(int GPUIndex){}
void amd::queryGPUPowerDraw(int GPUIndex){}
void amd::queryGPUPowerLimit(int GPUIndex){}
void amd::queryGPUPowerLimitLimits(int GPUIndex){}
void amd::queryGPUCurrentMaxClocks(int GPUIndex){}
void amd::queryGPUPowerLimitAvailability(int GPUIndex){}

bool amd::assignGPUFanSpeed(int GPUIndex, int targetValue){}
bool amd::assignGPUFanCtlMode(int GPUIndex, bool manual){}
bool amd::assignGPUFreqOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUMemClockOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUVoltageOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUPowerLimit(uint targetValue){}

#endif
