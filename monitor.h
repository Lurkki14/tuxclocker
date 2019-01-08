#ifndef MONITOR_H
#define MONITOR_H

#include <QMainWindow>

class monitor : public QMainWindow
{
    Q_OBJECT
public:
    explicit monitor(QWidget *parent = nullptr);
    QString temp;
    QString powerdraw;
    QString voltage;
    QString coreclock;
    QString memclock;
    QString coreutil;
    QString memutil;
    QString usedmem;
    QString totalmem;
    QString curMaxClk;

    QString nvTempQ = "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits";
    QString nvPowerDrawQ = "nvidia-smi --query-gpu=power.draw --format=csv,nounits,noheader";
    QString nvVoltQ = "nvidia-settings -q GPUCurrentCoreVoltage -t";
    QString nvCoreClkQ = "nvidia-smi --query-gpu=clocks.gr --format=csv,noheader,nounits";
    QString nvMemClkQ = "nvidia-smi --query-gpu=clocks.mem --format=csv,noheader,nounits";
    QString nvCoreUtilQ = "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits";
    QString nvMemUtilQ = "nvidia-smi --query-gpu=utilization.memory --format=csv,noheader,nounits";
    QString nvUsedMemQ = "nvidia-smi --query-gpu=memory.used --format=csv,noheader";
    QString nvTotalMemQ = "nvidia-smi --query-gpu=memory.total --format=csv,noheader";
    QString nvCurMaxClkQ = "nvidia-smi --query-supported-clocks=gr --format=csv,noheader";

public slots:
    void queryValues();
signals:

public slots:
private slots:
};

#endif // MONITOR_H
