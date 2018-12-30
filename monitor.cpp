#include "monitor.h"
#include "mainwindow.h"

monitor::monitor(QWidget *parent) : QMainWindow(parent)
{
}
void monitor::queryValues()
{
    QProcess process;
    process.start(nvTempQ);
    process.waitForFinished();
    temp = process.readLine();
    temp.chop(1);

    process.start(nvPowerDrawQ);
    process.waitForFinished();
    powerdraw = process.readLine();
    powerdraw.chop(1);

    process.start(nvVoltQ);
    process.waitForFinished();
    int voltnum = process.readLine().toInt()/1000;
    voltage = QString::number(voltnum);

    process.start(nvCoreClkQ);
    process.waitForFinished();
    coreclock = process.readLine();
    coreclock.chop(1);

    process.start(nvMemClkQ);
    process.waitForFinished();
    memclock = process.readLine();
    memclock.chop(1);

    process.start(nvCoreUtilQ);
    process.waitForFinished();
    coreutil = process.readLine();
    coreutil.chop(1);

    process.start(nvMemUtilQ);
    process.waitForFinished();
    memutil = process.readLine();
    memutil.chop(1);

    process.start(nvTotalMemQ);
    process.waitForFinished();
    totalmem = process.readLine();
    totalmem.chop(1);

    process.start(nvUsedMemQ);
    process.waitForFinished();
    usedmem = process.readLine();
    usedmem.chop(1);

    //process.start(nvCurMaxClkQ);
    //process.waitForFinished();
    //curMaxClk = process.readLine();
    //curMaxClk.chop(1);
}
