#ifndef NVIDIA_H
#define NVIDIA_H

#include <QObject>
#include <QDebug>
#include <QtX11Extras/QX11Info>

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
        bool overVoltAvailable;
        bool overClockAvailable;
        int maxVoltageOffset;

        int coreFreq;
        int memFreq;
        int temp;
        int voltage;
        int fanSpeed;
    };
    QVector <GPU> GPUList;
    int gpuCount = 0;
private:
    Display *dpy;
signals:

public slots:
    bool setupXNVCtrl();
    void queryGPUCount();
    void queryGPUNames();
    void queryGPUUIDs();
    void queryGPUFeatures();
    void queryGPUVoltage(int GPUIndex);
    void queryGPUTemp(int GPUIndex);
    void queryGPUFrequency(int GPUIndex);
    void queryGPUFanSpeed(int GPUIndex);

    bool assignFanSpeed(int GPUIndex, int targetValue);
private slots:
};

#endif // NVIDIA_H
