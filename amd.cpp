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
            amdgpu_device_handle *handle = new amdgpu_device_handle;
            int ret = amdgpu_device_initialize(fd, &major, &minor, handle);
            qDebug() << major;
            if (ret == 0) {
                // Create a gpu object with the correct paremeters
                GPU gpu;
                gpu.fsindex = i;
                gpu.gputype = Type::AMDGPU;
                // Get the hwmon folder name
                QString monpath = "/sys/class/drm/card"+QString::number(i)+"/device/hwmon";
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

                const char *name = amdgpu_get_marketing_name(*handle);
                char tempname[64];
                strcpy(tempname, name);
                gpu.name = tempname;
                gpu.displayName = QString::fromUtf8(name);
                gpu.dev = handle;
                qDebug() << gpu.name;

                int reading = 0;
                uint size = sizeof (int);
                ret = amdgpu_query_sensor_info(*handle, AMDGPU_INFO_SENSOR_GFX_SCLK, size, &reading);
                qDebug() << "coreclk" << reading << ret;

                GPUList.append(gpu);
                gpuCount++;

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
void amd::calculateUIProperties(int GPUIndex)
{
    GPUList[GPUIndex].voltageSliderMin = GPUList[GPUIndex].minVoltageLimit;
    GPUList[GPUIndex].voltageSliderMax = GPUList[GPUIndex].maxVoltageLimit;

    GPUList[GPUIndex].coreClkSliderMin = GPUList[GPUIndex].minCoreClkLimit;
    GPUList[GPUIndex].coreClkSliderMax = GPUList[GPUIndex].maxCoreClkLimit;

    GPUList[GPUIndex].memClkSliderMin = GPUList[GPUIndex].minMemClkLimit;
    GPUList[GPUIndex].memClkSliderMax = GPUList[GPUIndex].maxMemClkLimit;

    GPUList[GPUIndex].powerLimSliderMax = static_cast<int>(GPUList[GPUIndex].maxPowerLim);
    GPUList[GPUIndex].powerLimSliderMin = static_cast<int>(GPUList[GPUIndex].minPowerLim);

    if (GPUList[GPUIndex].overVoltAvailable) {
        GPUList[GPUIndex].voltageSliderCur = GPUList[GPUIndex].corevolts[GPUList[GPUIndex].corevolts.size()-1];
    }
    /*GPUList[GPUIndex].powerLimSliderCur = static_cast<int>(GPUList[GPUIndex].powerLim);*/
    if (GPUList[GPUIndex].overClockAvailable) {
        GPUList[GPUIndex].memClkSliderCur = GPUList[GPUIndex].memvolts[GPUList[GPUIndex].memclocks.size()-1];
        GPUList[GPUIndex].coreClkSliderCur = GPUList[GPUIndex].coreclocks[GPUList[GPUIndex].coreclocks.size()-1];
    }
}
void amd::calculateDisplayValues(int GPUIndex)
{
    GPUList[GPUIndex].displayTemp = GPUList[GPUIndex].temp/1000;
    GPUList[GPUIndex].displayPowerDraw = GPUList[GPUIndex].powerDraw;
    GPUList[GPUIndex].displayCoreFreq = GPUList[GPUIndex].coreFreq;
    GPUList[GPUIndex].displayMemFreq = GPUList[GPUIndex].memFreq;
    GPUList[GPUIndex].displayCoreUtil = static_cast <int> (GPUList[GPUIndex].coreUtil);
    // Not available on AMD
    GPUList[GPUIndex].displayMemUtil = 0;

    GPUList[GPUIndex].displayVoltage = GPUList[GPUIndex].voltage;
    GPUList[GPUIndex].displayFanSpeed = GPUList[GPUIndex].fanSpeed;
}
bool amd::setupGPUSecondary(int GPUIndex){return  true;}
void amd::queryGPUCount(){}
void amd::queryGPUNames()
{
    /*for (int i=0; i<GPUList.size(); i++) {
        if (GPUList[i].gputype == Type::AMDGPU) {
            const char *name;
            name = amdgpu_get_marketing_name(*GPUList[GPUIndex].dev);
            //strcpy(GPUList[i].name, name);
            qDebug() << name;
        }
    }*/
}
void amd::queryGPUUIDs(){}
void amd::queryGPUFeatures()
{
    // Read the pp_od_clk_voltage file and parse output
    QRegularExpression numexp("\\d+\\d");
    int type = 0;
    int column = 0;
    int breakcount = 0;
    QString path;
    QString line;
    for (int i=0; i<gpuCount; i++) {
        if (GPUList[i].gputype == Type::AMDGPU) {
            path = "/sys/class/drm/card"+QString::number(GPUList[i].fsindex)+"/device/pp_od_clk_voltage";
            QFile tablefile(path);
            bool ret = tablefile.open(QFile::ReadOnly | QFile::Text);
            if (ret) {
                QTextStream str(&tablefile);
                while (!str.atEnd() && breakcount < 30) {
                    line = str.readLine();
                    if (line.contains("OD_SCLK")) type = 1;
                    if (line.contains("OD_MCLK")) type = 2;
                    if (line.contains("OD_RANGE")) type = 3;
                    QRegularExpressionMatchIterator iter = numexp.globalMatch(line);
                    // Read all matches for the line
                    while  (iter.hasNext()) {
                        QRegularExpressionMatch nummatch = iter.next();
                        QString capline = nummatch.captured();
                        int num = capline.toInt();

                        if (type == 1) {
                            if (column == 0) {
                                GPUList[i].coreclocks.append(num);
                            } else {
                                GPUList[i].corevolts.append(num);
                            }
                        }

                        if (type == 2) {
                            if (column == 0) {
                                GPUList[i].memclocks.append(num);
                            } else {
                                GPUList[i].memvolts.append(num);
                            }
                        }

                        if (type == 3) {
                            if (line.contains("sclk", Qt::CaseInsensitive)) {
                                if (column == 0) {
                                    GPUList[i].minCoreClkLimit = num;
                                } else {
                                    GPUList[i].maxCoreClkLimit = num;
                                }
                            }
                            if (line.contains("mclk", Qt::CaseInsensitive)) {
                                if (column == 0) {
                                    GPUList[i].minMemClkLimit = num;
                                } else {
                                    GPUList[i].maxMemClkLimit = num;
                                }
                            }
                            if (line.contains("vdd", Qt::CaseInsensitive)) {
                                if (column == 0) {
                                    GPUList[i].minVoltageLimit = num;
                                } else {
                                    GPUList[i].maxVoltageLimit = num;
                                }
                            }
                        }
                        column++;
                    }
                    column = 0;
                    breakcount++;
                }
                tablefile.close();
            }
            // If the pstate vectors are empty after searching, set the features disabled
            if (!GPUList[i].corevolts.isEmpty()) GPUList[i].overVoltAvailable = true;

            if (!GPUList[i].coreclocks.isEmpty()) {
                GPUList[i].overClockAvailable = true;
                GPUList[i].memOverClockAvailable = true;
            }
            // Check if voltage is readable
            int reading;
            int retval = amdgpu_query_sensor_info(*GPUList[i].dev,
                                               AMDGPU_INFO_SENSOR_VDDGFX,
                                               sizeof (int),
                                               &reading);
            if (retval != 0) {
                GPUList[i].voltageReadable = false;
                qDebug() << "voltage unreadable for GPU" << i;
            } else {
                GPUList[i].voltageReadable = true;
            }
            // Core clock
            retval = amdgpu_query_sensor_info(*GPUList[i].dev,
                                               AMDGPU_INFO_SENSOR_GFX_SCLK,
                                               sizeof (int),
                                               &reading);
            if (retval != 0) {
                GPUList[i].coreClkReadable = false;
            } else {
                GPUList[i].coreClkReadable = true;
            }
            // Memory clock
            retval = amdgpu_query_sensor_info(*GPUList[i].dev,
                                               AMDGPU_INFO_SENSOR_GFX_MCLK,
                                               sizeof (int),
                                               &reading);
            if (retval != 0) {
                GPUList[i].memClkReadable = false;
            } else {
                GPUList[i].memClkReadable = true;
            }
            // GPU Utilization
            retval = amdgpu_query_sensor_info(*GPUList[i].dev,
                                               AMDGPU_INFO_SENSOR_GPU_LOAD,
                                               sizeof (int),
                                               &reading);
            if (retval != 0) {
                GPUList[i].coreUtilReadable= false;
                qDebug() << "utilization unreadable for GPU" << i;
            } else {
                GPUList[i].coreUtilReadable = true;
            }
            // Power draw
            retval = amdgpu_query_sensor_info(*GPUList[i].dev,
                                               AMDGPU_INFO_SENSOR_GPU_AVG_POWER,
                                               sizeof (int),
                                               &reading);
            if (retval != 0) {
                GPUList[i].powerDrawReadable = false;
            } else {
                GPUList[i].powerDrawReadable = true;
            }
            // Check if min/max power limits were readable (does this indicate it's writable though?)
            if (GPUList[i].minPowerLim == GPUList[i].maxPowerLim) {
                GPUList[i].powerLimitAvailable = false;
            } else {
                GPUList[i].powerLimitAvailable = true;
            }
        }
        // Assume manual fan control is always avilable for AMD
        GPUList[i].manualFanCtrlAvailable = true;
    }

}
void amd::queryGPUVoltage(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*GPUList[GPUIndex].dev,
                                       AMDGPU_INFO_SENSOR_VDDGFX,
                                       sizeof (GPUList[GPUIndex].voltage),
                                       &GPUList[GPUIndex].voltage);
    if (ret != 0) qDebug("Failed to query voltage");
}
void amd::queryGPUTemp(int GPUIndex)
{
    qDebug() << "querying GPU" << GPUIndex << GPUList[GPUIndex].displayName;
    int ret = amdgpu_query_sensor_info(*GPUList[GPUIndex].dev,
                             AMDGPU_INFO_SENSOR_GPU_TEMP,
                             sizeof (GPUList[GPUIndex].temp),
                             &GPUList[GPUIndex].temp);
    if (ret != 0) qDebug("Failed to query GPU temperature");
}
void amd::queryGPUFrequencies(int GPUIndex)
{
    int reading;
    int ret = amdgpu_query_sensor_info(*GPUList[GPUIndex].dev,
                                       AMDGPU_INFO_SENSOR_GFX_SCLK,
                                       sizeof (GPUList[GPUIndex].coreFreq),
                                       &GPUList[GPUIndex].coreFreq);
    qDebug() << reading << ret;
    if (ret != 0) qDebug("Failed to query GPU core clock");

    ret = amdgpu_query_sensor_info(*GPUList[GPUIndex].dev,
                                   AMDGPU_INFO_SENSOR_GFX_MCLK,
                                   sizeof (GPUList[GPUIndex].memFreq),
                                   &GPUList[GPUIndex].memFreq);
    if (ret != 0) qDebug("Failed to query GPU memory clock");
}
void amd::queryGPUFanSpeed(int GPUIndex)
{
    QFile pwmfile(GPUList[GPUIndex].hwmonpath+"/pwm1");
    bool ret = pwmfile.open(QFile::ReadOnly | QFile::Text);
    if (ret) {
        QString fanspeed = pwmfile.readLine().trimmed();
        double percspeed = (fanspeed.toDouble()/2.55);
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
    int ret = amdgpu_query_sensor_info(*GPUList[GPUIndex].dev,
                                       AMDGPU_INFO_SENSOR_GPU_LOAD,
                                       sizeof (GPUList[GPUIndex].coreUtil),
                                       &GPUList[GPUIndex].coreUtil);
    if (ret != 0) qDebug("Failed to query GPU Utilization");
}
void amd::queryGPUPowerDraw(int GPUIndex)
{
    int ret = amdgpu_query_sensor_info(*GPUList[GPUIndex].dev,
                                       AMDGPU_INFO_SENSOR_GPU_AVG_POWER,
                                       sizeof (GPUList[GPUIndex].powerDraw),
                                       &GPUList[GPUIndex].powerDraw);
    if (ret != 0) qDebug("failed to query GPU power draw");
    else qDebug() << GPUList[GPUIndex].powerDraw << "power draw";
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
    /*amdgpu_gpu_info info;
    int ret = amdgpu_query_gpu_info(*GPUList[GPUIndex].dev, &info);
    if (ret < 0) qDebug("Failed to query GPU maximum clocks");
    else {
        uint clock = static_cast<uint>(info.max_engine_clk);
        GPUList[GPUIndex].maxCoreClk = clock/1000;

        clock = static_cast<uint>(info.max_memory_clk);
        GPUList[GPUIndex].maxMemClk = clock/1000;
    }*/
}
void amd::queryGPUPowerLimitAvailability(int GPUIndex){}

bool amd::assignGPUFanSpeed(int GPUIndex, int targetValue)
{
    QProcess proc;
    bool ret = false;
    proc.start("pkexec /bin/sh -c \"echo '" + QString::number(targetValue*2.55) + "' > " + GPUList[GPUIndex].hwmonpath + "/pwm1");
    proc.waitForFinished(-1);
    if (proc.exitCode() == 0) {
        ret = true;
    }
    return ret;
}
bool amd::assignGPUFanCtlMode(int GPUIndex, bool manual)
{
    QProcess proc;
    bool ret = false;
    if (manual) {
        proc.start("pkexec /bin/sh -c \"echo '1' > " + GPUList[GPUIndex].hwmonpath + "/pwm1_enable");
        proc.waitForFinished(-1);
        if (proc.exitCode() == 0) {
            ret = true;
        }
    } else {
        proc.start("pkexec /bin/sh -c \"echo '0' > " + GPUList[GPUIndex].hwmonpath + "/pwm1_enable");
        proc.waitForFinished(-1);
        if (proc.exitCode() == 0) {
            ret = true;
        }
    }
    return ret;
}
bool amd::assignGPUFreqOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUMemClockOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUVoltageOffset(int GPUIndex, int targetValue){}
bool amd::assignGPUPowerLimit(uint targetValue){}

#endif
