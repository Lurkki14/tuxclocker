#ifdef AMD
#include "gputypes.h"

amd::amd() {}
bool amd::setupGPU()
{
    bool retb = false;
    qDebug("setting up amd gpu");
    // Check if the amdgpu driver is present on any of the cards in /dev/dri
    QDir devdir("/dev/dri");
    QStringList filelist = devdir.entryList();
    // First check for /dev/dri/renderD(128 + n)
    int fd = 0;
    for (int i=0; i<filelist.size(); i++) {
        QFileInfo info(devdir, "renderD"+QString::number(128+i));
        //qDebug() << info.exists();
        if (info.exists()) {
            // Get file descriptor for the device
            // Convert info.path to char
            char *path;
            QString abspath = "/dev/dri/renderD"+QString::number(128 + i);
            QByteArray arr = abspath.toLocal8Bit();
            path = arr.data();
            fd = open(path, O_RDONLY);
            qDebug() << info.fileName() << fd;

            // Attempt to initialize the GPU
            uint32_t major = 0;
            uint32_t minor = 0;
            amdgpu_device_handle handle;
            dev = &handle;
            int ret = amdgpu_device_initialize(fd, &major, &minor, dev);
            qDebug() << major;
            if (ret > -1) {
                // Create a gpu object with the correct paremeters
                GPU gpu;
                gpu.fsindex = i;
                gpu.gputype = Type::AMDGPU;
                // Get the hwmon folder name
                QString monpath = "/sys/class/drm/card0/device/hwmon";
                QDir mondir(monpath);
                qDebug() << mondir.entryList() << "mondir";
                QStringList list = mondir.entryList();
                for (int i=0; i<list.size(); i++) {
                    if (list[i].contains("hwmon")) {
                        qDebug() << list[i];
                        gpu.hwmonpath = monpath+"/"+list[i];
                        qDebug() << gpu.hwmonpath;
                        break;
                    }
                }

                const char *name = amdgpu_get_marketing_name(*dev);
                char tempname[64];
                strcpy(tempname, name);
                gpu.name = tempname;
                qDebug() << gpu.name;
                GPUList.append(gpu);

                retb = true;
            }
        }

    }
    if (!retb) {
        qDebug("No AMD GPUs using amdgpu found");
    } else {
        queryGPUTemp(0);
        queryGPUPowerLimit(0);
        queryGPUFanSpeed(0);
        queryGPUPowerLimitLimits(0);
    }
    return retb;
}
bool amd::setupGPUSecondary(int GPUIndex){return  true;}
void amd::queryGPUCount(){}
void amd::queryGPUNames()
{
    /*for (int i=0; i<GPUList.size(); i++) {
        if (GPUList[i].gputype == Type::AMDGPU) {
            const char *name;
            name = amdgpu_get_marketing_name(*dev);
            //strcpy(GPUList[i].name, name);
            qDebug() << name;
        }
    }*/
}
void amd::queryGPUUIDs(){}
void amd::queryGPUFeatures(){}
void amd::queryGPUVoltage(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*dev,
                                       AMDGPU_INFO_SENSOR_VDDGFX,
                                       sizeof (GPUList[GPUIndex].voltage),
                                       &GPUList[GPUIndex].voltage);
    if (ret < 0) qDebug("Failed to query voltage");
}
void amd::queryGPUTemp(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*dev,
                             AMDGPU_INFO_SENSOR_GPU_TEMP,
                             sizeof (GPUList[GPUIndex].temp),
                             &GPUList[GPUIndex].temp);
    if (ret < 0) qDebug("Failed to query GPU temperature");
}
void amd::queryGPUFrequencies(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*dev,
                                       AMDGPU_INFO_SENSOR_GFX_SCLK,
                                       sizeof (GPUList[GPUIndex].coreFreq),
                                       &GPUList[GPUIndex].coreFreq);
    if (ret < 0) qDebug("Failed to query GPU core clock");

    ret = amdgpu_query_sensor_info(*dev,
                                   AMDGPU_INFO_SENSOR_GFX_MCLK,
                                   sizeof (GPUList[GPUIndex].memFreq),
                                   &GPUList[GPUIndex].memFreq);
    if (ret < 0) qDebug("Failed to query GPU memory clock");
}
void amd::queryGPUFanSpeed(int GPUIndex)
{
    QFile pwmfile(GPUList[GPUIndex].hwmonpath+"/pwm1");
    bool ret = pwmfile.open(QFile::ReadOnly | QFile::Text);
    if (ret) {
        QString fanspeed = pwmfile.readLine().trimmed();
        double percspeed = (fanspeed.toDouble()/255)*100;
        GPUList[GPUIndex].fanSpeed = static_cast<int>(percspeed);
        qDebug() << GPUList[GPUIndex].fanSpeed << "fanspeed";
    }
    else qDebug("Failed to query fan speed");
}
void amd::queryGPUUsedVRAM(int GPUIndex){}
void amd::queryGPUFreqOffset(int GPUIndex){}
void amd::queryGPUMemClkOffset(int GPUIndex){}
void amd::queryGPUVoltageOffset(int GPUIndex){}

void amd::queryGPUUtils(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*dev,
                                       AMDGPU_INFO_SENSOR_GPU_LOAD,
                                       sizeof (GPUList[GPUIndex].coreUtil),
                                       &GPUList[GPUIndex].coreUtil);
    if (ret < 0) qDebug("Failed to query GPU Utilization");
}
void amd::queryGPUPowerDraw(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*dev,
                                       AMDGPU_INFO_SENSOR_GPU_AVG_POWER,
                                       sizeof (GPUList[GPUIndex].powerDraw),
                                       &GPUList[GPUIndex].powerDraw);
    if (ret < 0) qDebug("failed to query GPU power draw");
}
void amd::queryGPUPowerLimit(int GPUIndex)
{
    QFile pcapfile(GPUList[GPUIndex].hwmonpath+"/power1_cap");
    bool ret = pcapfile.open(QFile::ReadOnly | QFile::Text);
    if (ret) {
        QString pcap = pcapfile.readLine();
        GPUList[GPUIndex].powerLim = static_cast<uint>(pcap.toInt()/1000000);
        qDebug() << GPUList[GPUIndex].powerLim << "current power limit";
    }
    else qDebug("Failed to query power limit");
}
void amd::queryGPUPowerLimitLimits(int GPUIndex)
{
    QFile maxpcapfile(GPUList[GPUIndex].hwmonpath+"/power1_cap_max");
    bool ret = maxpcapfile.open(QFile::ReadOnly | QFile::Text);
    if (ret) {
        QString maxpcap = maxpcapfile.readLine();
        GPUList[GPUIndex].maxPowerLim = static_cast<uint>(maxpcap.toInt()/1000000);
        qDebug() << GPUList[GPUIndex].maxPowerLim << "max power limit";
    }

    QFile mincapfile(GPUList[GPUIndex].hwmonpath+"/power1_cap_min");
    ret = mincapfile.open(QFile::ReadOnly | QFile::Text);
    if (ret) {
        QString minpcap = mincapfile.readLine();
        GPUList[GPUIndex].minPowerLim = static_cast<uint>(minpcap.toInt()/1000000);
        qDebug() << GPUList[GPUIndex].minPowerLim << "min power limit";
    }
}
void amd::queryGPUCurrentMaxClocks(int GPUIndex)
{
    amdgpu_gpu_info info;
    int ret = amdgpu_query_gpu_info(*dev, &info);
    if (ret < 0) qDebug("Failed to query GPU maximum clocks");
    else {
        uint clock = static_cast<uint>(info.max_engine_clk);
        GPUList[GPUIndex].maxCoreClk = clock/1000;

        clock = static_cast<uint>(info.max_memory_clk);
        GPUList[GPUIndex].maxMemClk = clock/1000;
    }
}
void amd::queryGPUPowerLimitAvailability(int GPUIndex){}

bool amd::assignGPUFanSpeed(int GPUIndex, int targetValue){}
bool amd::assignGPUFanCtlMode(int GPUIndex, bool manual)
{

}
bool amd::assignGPUFreqOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUMemClockOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUVoltageOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUPowerLimit(uint targetValue){}

#endif
