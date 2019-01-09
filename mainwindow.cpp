#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editprofile.h"
#include "ui_editprofile.h"
#include "newprofile.h"
#include "monitor.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    checkForProfiles();
    queryGPUSettings();
    loadProfileSettings();
    queryDriverSettings();
    getGPUName();
    setupMonitorTab();
    setupGraphMonitorTab();
    tabHandler(ui->tabWidget->currentIndex());

    ui->frequencySlider->setRange(minCoreClkOfsInt, maxCoreClkOfsInt);
    ui->frequencySpinBox->setRange(minCoreClkOfsInt, maxCoreClkOfsInt);
    ui->frequencySlider->setValue(coreFreqOfsInt);
    ui->frequencySpinBox->setValue(coreFreqOfsInt);

    ui->powerLimSlider->setRange(minPowerLimInt, maxPowerLimInt);
    ui->powerLimSpinBox->setRange(minPowerLimInt, maxPowerLimInt);
    ui->powerLimSlider->setValue(curPowerLimInt);
    ui->powerLimSpinBox->setValue(curPowerLimInt);

    ui->memClkSlider->setRange(minMemClkOfsInt, maxMemClkOfsInt);
    ui->memClkSpinBox->setRange(minMemClkOfsInt, maxMemClkOfsInt);
    ui->memClkSlider->setValue(memClkOfsInt);
    ui->memClkSpinBox->setValue(memClkOfsInt);

    ui->voltageSlider->setRange(minVoltOfsInt, maxVoltOfsInt);
    ui->voltageSpinBox->setRange(minVoltOfsInt, maxVoltOfsInt);
    ui->voltageSlider->setValue(voltOfsInt);
    ui->voltageSpinBox->setValue(voltOfsInt);

    ui->fanSlider->setValue(fanSpeed);
    ui->fanSpinBox->setValue(fanSpeed);
    ui->fanSlider->setRange(0, 100);
    ui->fanSpinBox->setRange(0, 100);

    //QTimer *fanUpdateTimer = new QTimer(this);
    connect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(fanSpeedUpdater()));
    //connect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(tempUpdater()));
    fanUpdateTimer->start(2000);

    connect(ui->frequencySpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
    connect(ui->powerLimSpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
    connect(ui->memClkSpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));
    connect(ui->voltageSpinBox, SIGNAL(valueChanged(int)), SLOT(resetTimer()));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), SLOT(tabHandler(int)));
    connect(monitorUpdater, SIGNAL(timeout()), SLOT(updateMonitor()));

    plotCmds powerdrawplot;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionEdit_current_profile_triggered(bool)
{
    editProfile *editprof = new editProfile(this);
    editprof->show();
}

void MainWindow::on_pushButton_clicked()
{
    qDebug() << xCurvePoints;
}

void MainWindow::checkForRoot()
{
    QProcess process;
    process.start("/bin/sh -c \"echo $EUID\"");
    process.waitForFinished();
    QString EUID = process.readLine();
    if (EUID.toInt() == 0) {
        isRoot = true;
    }
    qDebug() << isRoot;
}
void MainWindow::tabHandler(int index)
{
    // Disconnect the monitor updater when the tab is not visible
    // Maybe do this with ifs if the tabs can be moved
    switch (index) {
        case 2:
            monitorUpdater->start(2000);
            break;
        case 1:
            monitorUpdater->start(1000);
            break;
        default:
            monitorUpdater->stop();
            break;
    }
}
void MainWindow::setupMonitorTab()
{
    // Set the behavior of the tree view
    ui->monitorTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->monitorTree->setIndentation(0);
    // Add the items
    gputemp->setText(0, "Core Temperature");
    powerdraw->setText(0, "Power Draw");
    voltage->setText(0, "Core Voltage");
    coreclock->setText(0, "Core Clock Frequency");
    memclock->setText(0, "Memory Clock Frequency");
    coreutil->setText(0, "Core Utilization");
    memutil->setText(0, "Memory Utilization");
    fanspeed->setText(0, "Fan Speed");
    memusage->setText(0, "Used Memory/Total Memory");
    curmaxclk->setText(0, "Maximum Core Clock Frequency");
    curmaxmemclk->setText(0, "Maximum Memory Clock Frequency");

    ui->monitorTree->addTopLevelItem(gputemp);
    ui->monitorTree->addTopLevelItem(powerdraw);
    ui->monitorTree->addTopLevelItem(voltage);
    ui->monitorTree->addTopLevelItem(coreclock);
    ui->monitorTree->addTopLevelItem(memclock);
    ui->monitorTree->addTopLevelItem(coreutil);
    ui->monitorTree->addTopLevelItem(memutil);
    ui->monitorTree->addTopLevelItem(fanspeed);
    ui->monitorTree->addTopLevelItem(memusage);
    ui->monitorTree->addTopLevelItem(curmaxclk);
    ui->monitorTree->addTopLevelItem(curmaxmemclk);
    // These values only change when the apply button has been pressed
    QString curMaxClk = QString::number(defCoreClk + latestClkOfs) + " MHz";
    curmaxclk->setText(1, curMaxClk);
    QString curMaxMemClk = QString::number(defMemClk + latestMemClkOfs) + " MHz";
    curmaxmemclk->setText(1, curMaxMemClk);
}
void MainWindow::setupGraphMonitorTab()
{
    monitor mon;
    mon.queryValues();
    fanSpeedUpdater();

    plotCmdsList.append(powerdrawplot);
    plotCmdsList.append(tempplot);
    plotCmdsList.append(coreclkplot);
    plotCmdsList.append(memclkplot);
    plotCmdsList.append(coreutilplot);
    plotCmdsList.append(memutilplot);
    plotCmdsList.append(voltageplot);
    plotCmdsList.append(fanspeedplot);
    // Layout for the plots
    plotWidget->setLayout(plotLayout);

    // Define the structs
    //plotCmdsList[0].valueq = mon.temp.toDouble();
    plotCmdsList[0].plot = tempPlot;
    plotCmdsList[0].vector = qv_temp;
    plotCmdsList[0].layout = tempLayout;
    plotCmdsList[0].widget = tempWidget;
    //plotCmdsList[0].mintext = tempMinText;
    //plotCmdsList[0].maxtext = tempMaxText;

    //plotCmdsList[1].valueq = mon.powerdraw.toDouble();
    plotCmdsList[1].plot = powerDrawPlot;
    plotCmdsList[1].vector = qv_powerDraw;
    plotCmdsList[1].layout = powerDrawLayout;
    plotCmdsList[1].widget = powerDrawWidget;
    //plotCmdsList[1].mintext = powerDrawMinText;
    //plotCmdsList[1].maxtext = powerDrawMaxText;

    //plotCmdsList[2].valueq = mon.coreclock.toDouble();
    plotCmdsList[2].plot = coreClkPlot;
    plotCmdsList[2].vector = qv_coreClk;
    plotCmdsList[2].layout = coreClkLayout;
    plotCmdsList[2].widget = coreClkWidget;
    //plotCmdsList[2].mintext = coreClkMinText;
    //plotCmdsList[2].maxtext = coreClkMaxText;

    //plotCmdsList[3].valueq = mon.memclock.toDouble();
    plotCmdsList[3].plot = memClkPlot;
    plotCmdsList[3].vector = qv_memClk;
    plotCmdsList[3].layout = memClkLayout;
    plotCmdsList[3].widget = memClkWidget;
    //plotCmdsList[3].mintext = memClkMinText;
    //plotCmdsList[3].maxtext = memClkMaxText;

    plotCmdsList[4].plot = coreUtilPlot;
    plotCmdsList[4].vector = qv_coreUtil;
    plotCmdsList[4].layout = coreUtilLayout;
    plotCmdsList[4].widget = coreUtilWidget;
    //plotCmdsList[4].mintext = coreUtilMinText;
    //plotCmdsList[4].maxtext = coreUtilMaxText;

    plotCmdsList[5].plot = memUtilPlot;
    plotCmdsList[5].vector = qv_memUtil;
    plotCmdsList[5].layout = memUtilLayout;
    plotCmdsList[5].widget = memUtilWidget;
    //plotCmdsList[5].mintext = memUtilMinText;
    //plotCmdsList[6].maxtext = memUtilMaxText;

    plotCmdsList[6].plot = voltagePlot;
    plotCmdsList[6].vector = qv_voltage;
    plotCmdsList[6].layout = voltageLayout;
    plotCmdsList[6].widget = voltageWidget;
    //plotCmdsList[7].mintext = voltageMinText;
    //plotCmdsList[7].maxtext = voltageMaxText;

    plotCmdsList[7].plot = fanSpeedPlot;
    plotCmdsList[7].vector = qv_fanSpeed;
    plotCmdsList[7].layout = fanSpeedLayout;
    plotCmdsList[7].widget = fanSpeedWidget;
    //plotCmdsList[7].mintext = fanMinText;
    //plotCmdsList[7].maxtext = fanMaxText;

    plotCmdsList[0].valueq = mon.temp.toDouble();
    plotCmdsList[1].valueq = mon.powerdraw.toDouble();
    plotCmdsList[2].valueq = mon.coreclock.toDouble();
    plotCmdsList[3].valueq = mon.memclock.toDouble();
    plotCmdsList[4].valueq = mon.coreutil.toDouble();
    plotCmdsList[5].valueq = mon.memutil.toDouble();
    plotCmdsList[6].valueq = mon.voltage.toDouble();
    plotCmdsList[7].valueq = fanSpeed;
    // Get the main widget backgorund palette and use the colors for the plots
    QPalette palette;
    palette.setCurrentColorGroup(QPalette::Active);
    QColor color = palette.color(QPalette::Background);
    QColor textColor = palette.color(QPalette::Text);
    QColor graphColor = palette.color(QPalette::Highlight);
    QPen graphPen;
    graphPen.setWidth(2);
    graphPen.setColor(graphColor);
    QPen tickPen;
    tickPen.setWidthF(0.5);
    tickPen.setColor(textColor);
    // Define features common to all plots

    for (int i=0; i<plotCmdsList.size(); i++) {
        plotCmdsList[i].plot->setMinimumHeight(220);
        plotCmdsList[i].plot->setMaximumHeight(220);
        plotCmdsList[i].plot->setMinimumWidth(200);

        plotCmdsList[i].plot->addGraph();
        plotCmdsList[i].plot->xAxis->setRange(-plotVectorSize +1, 0);
        plotCmdsList[i].plot->xAxis->setLabel("Time (s)");

        // Add the widget to the main layout
        plotCmdsList[i].widget->setLayout(plotCmdsList[i].layout);
        plotCmdsList[i].layout->addWidget(plotCmdsList[i].plot);
        plotLayout->addWidget(plotCmdsList[i].widget);

        plotCmdsList[i].plot->setBackground(color);
        plotCmdsList[i].plot->xAxis->setLabelColor(textColor);
        plotCmdsList[i].plot->yAxis->setLabelColor(textColor);
        plotCmdsList[i].plot->xAxis->setTickLabelColor(textColor);
        plotCmdsList[i].plot->yAxis->setTickLabelColor(textColor);
        plotCmdsList[i].plot->graph(0)->setPen(graphPen);
        plotCmdsList[i].plot->xAxis->setTickPen(tickPen);
        plotCmdsList[i].plot->yAxis->setTickPen(tickPen);
        plotCmdsList[i].plot->xAxis->setSubTickPen(tickPen);
        plotCmdsList[i].plot->yAxis->setSubTickPen(tickPen);
        plotCmdsList[i].plot->xAxis->setBasePen(tickPen);
        plotCmdsList[i].plot->yAxis->setBasePen(tickPen);

        QCPTextElement *minelem = new QCPTextElement(plotCmdsList[i].plot);
        plotCmdsList[i].mintext = minelem;
        minelem->setText("Min: " + QString::number(plotCmdsList[i].valueq));
        minelem->setTextColor(textColor);

        QCPTextElement *maxelem = new QCPTextElement(plotCmdsList[i].plot);
        plotCmdsList[i].maxtext = maxelem;
        maxelem->setText("Max: " + QString::number(plotCmdsList[i].valueq));
        maxelem->setTextColor(textColor);

        plotCmdsList[i].plot->plotLayout()->insertRow(0);
        QCPLayoutGrid *sublo = new QCPLayoutGrid;
        plotCmdsList[i].plot->plotLayout()->addElement(0, 0, sublo);
        sublo->setMargins(QMargins(5 ,5, 5, 5));
        sublo->addElement(plotCmdsList[i].mintext);
        sublo->addElement(plotCmdsList[i].maxtext);

        //QCPLayoutGrid *sublo = new QCPLayoutGrid;
        //plotCmdsList[i].plot->plotLayout()->addElement(0, 0, sublo);
        //sublo->setMargins(QMargins(5 ,5, 5, 5));
        //sublo->addElement(plotCmdsList[i].mintext);
        //sublo->addElement(plotCmdsList[i].maxtext);
        //plotCmdsList[i].maxtext->setTextColor(textColor);
        //plotCmdsList[i].mintext->setTextColor(textColor);
        //plotCmdsList[i].plot->plotLayout()->addElement(plotCmdsList[i].maxtext);
        //plotCmdsList[i].maxtext->setText("Max: " + QString::number(plotCmdsList[i].valueq));
        //plotCmdsList[i].mintext->setText("Min: " + QString::number(plotCmdsList[i].valueq));

        // Set the y-range
        plotCmdsList[i].plot->yAxis->setRange(plotCmdsList[i].valueq -plotCmdsList[i].valueq*0.1, plotCmdsList[i].valueq + plotCmdsList[i].valueq*0.1);
        //connect(plotCmdsList[i].plot, SIGNAL(mouseMove(QMouseEvent*)), SLOT(plotHovered(QMouseEvent*)));
        //connect(plotCmdsList[i].widget, SIGNAL(mouseEnterEvent(QEvent*)), SLOT(plotHovered()));

        // Add the tracers
        QCPItemTracer *mouseTracer = new QCPItemTracer(plotCmdsList[i].plot);
        plotCmdsList[i].tracer = mouseTracer;
        mouseTracer->setStyle(QCPItemTracer::tsCrosshair);
        QPen tracerPen = graphPen;
        tracerPen.setWidthF(0.5);
        mouseTracer->setPen(tracerPen);
        //connect(r, SIGNAL(mouseMove(QMouseEvent*)), SLOT(plotHovered(QMouseEvent*)));
        connect(plotCmdsList[i].plot, SIGNAL(mouseMove(QMouseEvent*)), SLOT(plotHovered(QMouseEvent*)));
    }

    tempPlot->yAxis->setLabel("Temperature (°C)");
    powerDrawPlot->yAxis->setLabel("Power Draw (W)");
    coreClkPlot->yAxis->setLabel("Core Clock Frequency (MHz)");
    memClkPlot->yAxis->setLabel("Memory Clock Frequency (MHz)");
    coreUtilPlot->yAxis->setLabel("Core Utilization (%)");
    memUtilPlot->yAxis->setLabel("Memory Utilization (%)");
    voltagePlot->yAxis->setLabel("Core Voltage (mV)");
    fanSpeedPlot->yAxis->setLabel("Fan Speed (%)");

    plotScrollArea->setWidget(plotWidget);
    plotScrollArea->setWidgetResizable(true);

    // Add scroll area to a layout so we can set it as the widget for the tab
    lo->addWidget(plotScrollArea);
    ui->monitorTab->setLayout(lo);

    connect(plotHoverUpdater, SIGNAL(timeout()), SLOT(plotHovered()));
}
void MainWindow::updateMonitor()
{
    monitor mon;
    mon.queryValues();
    fanSpeedUpdater();
    gputemp->setText(1, mon.temp + "°C");
    powerdraw->setText(1, mon.powerdraw + "W");
    voltage->setText(1, mon.voltage + "mV");
    coreclock->setText(1, mon.coreclock + " MHz");
    memclock->setText(1, mon.memclock + " MHz");
    coreutil->setText(1, mon.coreutil + " %");
    memutil->setText(1, mon.memutil + " %");
    fanspeed->setText(1, QString::number(fanSpeed) + " %");
    memusage->setText(1, mon.usedmem + "/" + mon.totalmem);


    // Decrement all time values by one
    for (int i=0; i<qv_time.length(); i++) {
        qv_time[i]--;
    }
    // Add current time (0)
    if (qv_time.size() < plotVectorSize) {
        qv_time.append(0);
    } else {
        qv_time.insert(plotVectorSize, 0);
    }
    // Remove the first elements if there are more elements than the x-range
    if (qv_time.size() > plotVectorSize) {
        qv_time.removeFirst();
    }
    // Update the values for plots
    plotCmdsList[0].valueq = mon.temp.toDouble();
    plotCmdsList[1].valueq = mon.powerdraw.toDouble();
    plotCmdsList[2].valueq = mon.coreclock.toDouble();
    plotCmdsList[3].valueq = mon.memclock.toDouble();
    plotCmdsList[4].valueq = mon.coreutil.toDouble();
    plotCmdsList[5].valueq = mon.memutil.toDouble();
    plotCmdsList[6].valueq = mon.voltage.toDouble();
    plotCmdsList[7].valueq = fanSpeed;
    for (int i=0; i<plotCmdsList.size(); i++) {
        // Check if the max/min values need to be updated
        if (!plotCmdsList[i].vector.isEmpty()) {
            double lowestval = plotCmdsList[i].vector[0];
            double largestval = plotCmdsList[i].vector[0];
            for (int j=0; j<plotCmdsList[i].vector.size(); j++) {
                if (plotCmdsList[i].vector[j] < lowestval) {
                    lowestval = plotCmdsList[i].vector[j];
                }
                if (plotCmdsList[i].vector[j] > largestval) {
                    largestval = plotCmdsList[i].vector[j];
                }
            }
            if (largestval < plotCmdsList[i].valueq) {
                plotCmdsList[i].maxtext->setText("Max: " + QString::number(plotCmdsList[i].valueq));
            }
            if (lowestval > plotCmdsList[i].valueq) {
                plotCmdsList[i].mintext->setText("Min: " + QString::number(plotCmdsList[i].valueq));
            }
        }
        if (plotCmdsList[i].vector.size() < plotVectorSize) {
            plotCmdsList[i].vector.append(plotCmdsList[i].valueq);
        } else {
            plotCmdsList[i].vector.insert(plotVectorSize, plotCmdsList[i].valueq);
        }
        // Remove the first element if there are more elements than the x-range
        if (plotCmdsList[i].vector.size() > plotVectorSize) {
            plotCmdsList[i].vector.removeFirst();
        }
        plotCmdsList[i].plot->graph(0)->setData(qv_time, plotCmdsList[i].vector);
        // If the newest value is out of bounds, resize the y-range
        if (plotCmdsList[i].valueq > plotCmdsList[i].plot->yAxis->range().upper) {
            plotCmdsList[i].plot->yAxis->setRangeUpper(plotCmdsList[i].valueq + plotCmdsList[i].valueq*0.1);
        }
        if (plotCmdsList[i].valueq < plotCmdsList[i].plot->yAxis->range().lower) {
            plotCmdsList[i].plot->yAxis->setRangeLower(plotCmdsList[i].valueq - plotCmdsList[i].valueq*0.1);
        }
        plotCmdsList[i].plot->replot();
        plotCmdsList[i].plot->update();
    }
    // If the largest/smallest value is too far from the range end, this resizes them every 10th iteration of this function
    if (counter >= 10) {
        for (int i=0; i<plotCmdsList.size(); i++) {
            double lowestval = plotCmdsList[i].vector[0];
            double largestval = plotCmdsList[i].vector[0];
            for (int j=0; j<plotCmdsList[i].vector.size(); j++) {
                if (plotCmdsList[i].vector[j] < lowestval) {
                    lowestval = plotCmdsList[i].vector[j];
                }
                if (plotCmdsList[i].vector[j] > largestval) {
                    largestval = plotCmdsList[i].vector[j];
                }
            }
            if (plotCmdsList[i].plot->yAxis->range().upper - largestval*0.15 > largestval) {
                plotCmdsList[i].plot->yAxis->setRange(lowestval - lowestval*0.1, largestval + largestval*0.1);
            }
            if (plotCmdsList[i].plot->yAxis->range().lower + lowestval*0.15 < lowestval) {
                // Don't set the lower range to under 0
                if (lowestval - lowestval*0.1 < 0) {
                    plotCmdsList[i].plot->yAxis->setRangeLower(0);
                } else {
                    plotCmdsList[i].plot->yAxis->setRange(lowestval - lowestval*0.1, largestval + largestval*0.1);
                }
            }
        }
        counter = 0;
    }
    counter++;
}
void MainWindow::plotHovered(QMouseEvent *event)
{
    //plotHoverUpdater->start(1000);
    QPoint cursor = event->pos();
    //qDebug() << childAt(cursor);
    //QWidget *widget = childAt(cursor);
    //QCustomPlot *plot = widget->findChild<QCustomPlot*>(QString(), Qt::FindDirectChildrenOnly);
    //plot->xAxis->setRange(-15, 0);

    int plotIndex = 0;
    for (int i=0; i<plotCmdsList.size(); i++) {
        if (plotCmdsList[i].widget->underMouse()) {
            plotIndex = i;
            break;
        }
    }
    //int xindex = round(plotCmdsList[plotIndex].plot->xAxis->pixelToCoord(cursor.x())) ;
    double pointerxcoord = plotCmdsList[plotIndex].plot->xAxis->pixelToCoord(cursor.x());
    //double pointerycoord = plotCmdsList[plotIndex].plot->yAxis->pixelToCoord(cursor.y());
    plotCmdsList[plotIndex].tracer->position->setCoords(pointerxcoord, plotCmdsList[plotIndex].plot->yAxis->range().upper);
    // Find the y-value for the corresponding coordinate
    //double ycoord = plotCmdsList[plotIndex].vector[xindex];
    //qDebug() << xindex;
    //qDebug() << plotCmdsList[plotIndex].plot->plottableAt(event->pos());
    //QPointF pixelPos = QCPAbstractPlottable1D<>()
    //int pointerxint = round(pointerxcoord);
    int valIndex = 0;
    if (!qv_time.isEmpty()) {
        for (int i=0; i<plotCmdsList[plotIndex].vector.size(); i++) {
            double deltax = abs(qv_time[0] - pointerxcoord);
            if (abs(qv_time[i] - pointerxcoord) < deltax) {
                valIndex = i;
            }
        }
    }
    qDebug() << plotCmdsList[plotIndex].vector[valIndex];
    plotCmdsList[plotIndex].plot->update();
    plotCmdsList[plotIndex].plot->replot();
    QThread::msleep(5);
}
void MainWindow::checkForProfiles()
{
    qDebug() << "chkproffunc";
    // If there are no profiles, create one, then list all the entries whose isProfile is true in the profile selection combo box
    QSettings settings("nvfancurve");
    QStringList groups = settings.childGroups();
    QString isProfile = "/isProfile";

    for (int i=0; i<groups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = groups[i];
        QString query = group.append(isProfile);
        if (settings.value(query).toBool()) {
            noProfiles = false;
            break;
        }
    }
    if (noProfiles) {
        settings.setValue("New Profile/isProfile", true);
        settings.setValue("General/currentProfile", "New Profile");
        currentProfile = "New Profile";
    }
    // Redefine child groups so it contains the newly created profile if it was made
    QStringList newgroups = settings.childGroups();
    for (int i=0; i<newgroups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = newgroups[i];
        QString query = group.append(isProfile);
        if (settings.value(query).toBool()) {
            ui->profileComboBox->addItem(newgroups[i]);
        }
    }
}

void MainWindow::on_profileComboBox_activated(const QString &arg1)
{
    // Change currentProfile to combobox selection
    currentProfile = arg1;
}

void MainWindow::getGPUDriver()
{
    QProcess process;
    process.start(queryForNvidiaProp);
    process.waitForFinished(-1);
    if (process.readAllStandardOutput().toInt() >= 1) {
        gpuDriver = "nvidia";
    }
}

void MainWindow::getGPUName()
{
    QProcess process;
    process.start(queryGPUName);
    process.waitForFinished(-1);
    ui->GPUNameLabel->setText(process.readLine());
}

void MainWindow::fanSpeedUpdater()
{
    QProcess process;
    process.start(nvFanQ);
    process.waitForFinished(-1);
    fanSpeed = process.readLine().toInt();
    ui->fanSlider->setValue(fanSpeed);
    ui->fanSpinBox->setValue(fanSpeed);
}
void MainWindow::tempUpdater()
{
    QProcess process;
    process.start(nvTempQ);
    process.waitForFinished(-1);
    temp = process.readLine().toInt();
    if (xCurvePoints.size() != 0) {
        generateFanPoint();
    }
}
void MainWindow::resetTimer()
{
    // If a value has been changed this timer will start. When the apply button has been pressed, this gets cancelled
    connect(resettimer, SIGNAL(timeout()), SLOT(resetChanges()));
    resettimer->stop();
    resettimer->setSingleShot(true);
    resettimer->start(10000);
}
void MainWindow::resetStatusLabel()
{
    ui->statusLabel->clear();
}
void MainWindow::resetChanges()
{
    // If the settings haven't been applied in 10 seconds, reset all values to their latest values
    ui->frequencySlider->setValue(latestClkOfs);
    ui->frequencySpinBox->setValue(latestClkOfs);

    ui->powerLimSlider->setValue(latestPowerLim);
    ui->powerLimSpinBox->setValue(latestPowerLim);

    ui->voltageSlider->setValue(latestVoltOfs);
    ui->voltageSpinBox->setValue(latestVoltOfs);

    ui->memClkSlider->setValue(latestMemClkOfs);
    ui->memClkSpinBox->setValue(latestMemClkOfs);
    qDebug() << "timer";
}
void MainWindow::queryDriverSettings()
{
    QProcess process;
    process.start(nvFanCtlStateQ);
    process.waitForFinished(-1);
    if (process.readLine().toInt() == 1) {
        manualFanCtl = true;
        ui->fanModeComboBox->setCurrentIndex(1);
    } else {
        manualFanCtl = false;
        ui->fanModeComboBox->setCurrentIndex(0);
    }
}
void MainWindow::applyFanMode()
{
    QProcess process;
    switch (fanControlMode) {
        case 0:
            // Driver controlled mode
            process.start(nvFanCtlStateSet + "0");
            process.waitForFinished(-1);
            ui->fanSlider->setEnabled(false);
            ui->fanSpinBox->setEnabled(false);
            break;
        case 1:
           // Static mode
            process.start(nvFanCtlStateSet + "1");
            process.waitForFinished(-1);
            disconnect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(tempUpdater()));
            process.start(nvFanSpeedSet + QString::number(ui->fanSlider->value()));
            process.waitForFinished(-1);
            ui->fanSlider->setEnabled(true);
            ui->fanSpinBox->setEnabled(true);
            break;
        case 2:
            // Custom mode
            process.start(nvFanCtlStateSet + "1");
            process.waitForFinished(-1);
            connect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(tempUpdater()));
            ui->fanSlider->setEnabled(false);
            ui->fanSpinBox->setEnabled(false);
            break;
    }
}
void MainWindow::queryGPUSettings()
{
    QProcess process;
    process.start(nvVoltQ);
    process.waitForFinished(-1);
    voltInt = process.readLine().toInt()/1000;

    process.start(nvVoltOfsQ);
    process.waitForFinished(-1);
    voltOfsInt = process.readLine().toInt()/1000;
    latestVoltOfs = voltOfsInt;

    defVolt = voltInt - voltOfsInt;

    process.start(nvVoltOfsLimQ);
    process.waitForFinished(-1);
    for (int i=0; i<process.size(); i++) {
        if (process.readLine().toInt()/1000 > maxVoltOfsInt) {
            qDebug() << process.readLine();
            maxVoltOfsInt = process.readLine().toInt()/1000;
        }
    }

    process.start(nvCoreClkOfsQ);
    process.waitForFinished(-1);
    coreFreqOfsInt = process.readLine().toInt();
    latestClkOfs = coreFreqOfsInt;

    process.start(nvCurMaxClkQ);
    process.waitForFinished(-1);
    curMaxClkInt = process.readLine().toInt();
    qDebug() << curMaxClkInt;

    process.start(nvMaxPowerLimQ);
    process.waitForFinished(-1);
    maxPowerLimInt = process.readLine().toInt();

    process.start(nvMinPowerLimQ);
    process.waitForFinished(-1);
    minPowerLimInt = process.readLine().toInt();

    process.start(nvCurPowerLimQ);
    process.waitForFinished(-1);
    curPowerLimInt = process.readLine().toInt();
    latestPowerLim = curPowerLimInt;

    process.start(nvClockLimQ);
    process.waitForFinished(-1);
    for (int i=0; i<process.size(); i++) {
        QString line = process.readLine();
        if (line.toInt() > maxCoreClkOfsInt) {
            maxCoreClkOfsInt = line.toInt();
        }
        if (line.toInt() <= minCoreClkOfsInt) {
            minCoreClkOfsInt = line.toInt();
        }
    }

    // This gets the transfer rate, the clock speed is rate/2
    process.start(nvMemClkLimQ);
    process.waitForFinished(-1);
    for (int i=0; i<process.size(); i++) {
        QString line = process.readLine();
        if (line.toInt()/2 > maxMemClkOfsInt) {
            maxMemClkOfsInt = line.toInt()/2;
        }
        if (line.toInt()/2 <= minMemClkOfsInt) {
            minMemClkOfsInt = line.toInt()/2;
        }
    }

    process.start(nvCurMaxMemClkQ);
    process.waitForFinished(-1);
    curMaxMemClkInt = process.readLine().toInt();

    process.start(nvCurMemClkOfsQ);
    process.waitForFinished(-1);
    memClkOfsInt = process.readLine().toInt()/2;
    latestMemClkOfs = memClkOfsInt;

    // Since the maximum core clock reported is the same on negative offsets as on 0 offset add a check here
    if (0 >= coreFreqOfsInt) {
        defCoreClk = curMaxClkInt;
    } else {
        defCoreClk = curMaxClkInt - coreFreqOfsInt;
    }
    defMemClk = curMaxMemClkInt - memClkOfsInt;

}
void MainWindow::applyGPUSettings()
{
    QProcess process;
    int offsetValue;
    int powerLimit;
    bool hadErrors = false;
    errorText = "Failed to apply these settings: ";
    QString input = nvCoreClkSet;
    if (latestClkOfs != ui->frequencySlider->value()) {
        offsetValue = ui->frequencySlider->value();
        QString input = nvCoreClkSet;
        input.append(QString::number(offsetValue));
        qDebug() << input;
        process.start(input);
        process.waitForFinished(-1);
        // Check if the setting got applied by querying nvidia-settings (very inefficient unfortunately)
        process.start(nvCoreClkOfsQ);
        process.waitForFinished();
        if (process.readLine().toInt() == ui->frequencySlider->value()) {
            latestClkOfs = ui->frequencySlider->value();
        } else {
            errorText.append("- Core Clock Offset ");
            hadErrors = true;
        }
    }

    if (latestMemClkOfs != ui->memClkSlider->value()) {
        offsetValue = ui->memClkSlider->value();
        input = nvMemClkSet;
        input.append(QString::number(offsetValue*2));
        qDebug() << input;
        process.start(input);
        process.waitForFinished(-1);

        process.start(nvCurMemClkOfsQ);
        process.waitForFinished(-1);
        if (process.readLine().toInt()/2 == ui->memClkSlider->value()) {
            latestMemClkOfs = ui->memClkSlider->value();
        } else {
            errorText.append("- Memory Clock Offset");
            hadErrors = true;
        }
    }

    if (latestPowerLim != ui->powerLimSlider->value()) {
        powerLimit = ui->powerLimSlider->value();
        input = nvPowerLimSet;
        if (!isRoot) {
            input = "/bin/sh -c \"pkexec " + input + QString::number(powerLimit) + "\"";
            qDebug() << input;
            process.start(input);
            process.waitForFinished(-1);
            if (process.exitCode() != 0) {
                errorText.append("- Power Limit ");
                hadErrors = true;
                ui->powerLimSlider->setValue(latestPowerLim);
            } else {
                latestPowerLim = ui->powerLimSlider->value();
            }


        } else {
            input.append(QString::number(powerLimit));
            process.start(input);
            process.waitForFinished(-1);
            qDebug() << "ran as root";
        }
    }

    if (latestVoltOfs != ui->voltageSlider->value()) {
        offsetValue = ui->voltageSlider->value();
        input = nvVoltageSet;
        input.append(QString::number(offsetValue*1000));
        qDebug() << input;
        process.start(input);
        process.waitForFinished(-1);

        process.start(nvVoltOfsQ);
        process.waitForFinished(-1);
        if (process.readLine().toInt()/1000 == ui->voltageSlider->value()) {
            latestVoltOfs = ui->voltageSlider->value();
        } else {
            errorText.append("- Voltage Offset");
            hadErrors = true;
        }
    }
    if (hadErrors) {
        ui->statusLabel->setText(errorText);
    } else {
        ui->statusLabel->setText("Settings applied");
    }
    resettimer->stop();
}

void MainWindow::loadProfileSettings()
{
    QSettings settings("nvfancurve");
    currentProfile = settings.value("General/currentProfile").toString();
    settings.beginGroup(currentProfile);
    // Check for existance of the setting so zeroes don't get appended to curve point vectors
    if (settings.contains("xpoints")) {
        QString xPointStr = "/bin/sh -c \"echo " + settings.value("xpoints").toString() + grepStringToInt;
        QString yPointStr = "/bin/sh -c \"echo " + settings.value("ypoints").toString() + grepStringToInt;
        QProcess process;
        process.start(xPointStr);
        process.waitForFinished(-1);
        for (int i=0; i<process.size() +1; i++) {
            xCurvePoints.append(process.readLine().toInt());
        }
        process.start(yPointStr);
        process.waitForFinished(-1);
        for (int i=0; i<process.size() +1; i++) {
            yCurvePoints.append(process.readLine().toInt());
        }
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->fanModeComboBox->model());
        QModelIndex customModeIndex = model->index(2, ui->fanModeComboBox->modelColumn());
        QStandardItem *customMode = model->itemFromIndex(customModeIndex);
        customMode->setEnabled(true);
        customMode->setToolTip("Use your own fan curve");
    } else {
        // Set fan mode "Custom" unselectable if there are no custom curve points
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->fanModeComboBox->model());
        QModelIndex customModeIndex = model->index(2, ui->fanModeComboBox->modelColumn());
        QStandardItem *customMode = model->itemFromIndex(customModeIndex);
        customMode->setEnabled(false);
        customMode->setToolTip("To use this mode you must make a fan curve first");
    }
    if (settings.contains("voltageOffset")) {
        latestVoltOfs = settings.value("voltageOffset").toInt();
    }
    if (settings.contains("powerLimit")) {
        latestPowerLim = settings.value("powerLimit").toInt();
    }
    if (settings.contains("clockFrequencyOffset")) {
        latestClkOfs = settings.value("clockFrequencyOffset").toInt();
    }
    if (settings.contains("memoryClockOffset")) {
        latestMemClkOfs=settings.value("memoryClockOffset").toInt();
    }
    if (settings.contains("fanControlMode")) {
        fanControlMode = settings.value("fanControlMode").toInt();
        ui->fanModeComboBox->setCurrentIndex(fanControlMode);
    }
    ui->statusLabel->setText("Profile settings loaded.");
    statusLabelResetTimer->start(7000);
    statusLabelResetTimer->setSingleShot(true);
    connect(statusLabelResetTimer, SIGNAL(timeout()), SLOT(resetStatusLabel()));
    qDebug() << xCurvePoints << yCurvePoints;
}

void MainWindow::on_newProfile_closed()
{
    qDebug() << "dialog closed";
    ui->profileComboBox->clear();
    QSettings settings ("nvfancurve");
    QString isProfile = "/isProfile";
    // Refresh the profile combo box
    QStringList newgroups = settings.childGroups();
    for (int i=0; i<newgroups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = newgroups[i];
        QString query = group.append(isProfile);
        if (settings.value(query).toBool()) {
            ui->profileComboBox->addItem(newgroups[i]);
        }
    }
}
void MainWindow::saveProfileSettings()
{
    QSettings settings("nvfancurve");
    settings.beginGroup(currentProfile);
    settings.setValue("voltageOffset", latestVoltOfs);
    settings.setValue("powerLimit", latestPowerLim);
    settings.setValue("clockFrequencyOffset", latestClkOfs);
    settings.setValue("memoryClockOffset", latestMemClkOfs);
    settings.setValue("fanControlMode", fanControlMode);
}

void MainWindow::generateFanPoint()
{
    // Calculate the value for fan speed based on temperature
    // First check if the fan speed should be y[0] or y[final]
    int index = 0;
    if (temp <= xCurvePoints[0]) {
        targetFanSpeed = yCurvePoints[0];
    }
    if (temp >= xCurvePoints[xCurvePoints.size()-1]) {
        targetFanSpeed = yCurvePoints[yCurvePoints.size()-1];
    } else {
        // Get the index of the leftmost point of the interpolated interval by comparing it to temperature
        for (int i=0; i<xCurvePoints.size(); i++) {
            if (temp >= xCurvePoints[i] && temp <= xCurvePoints[i+1]) {
                index = i;
                break;
            }
        }
        // Check if the change in x is zero to avoid dividing by it
        if (xCurvePoints[index] - xCurvePoints[index + 1] == 0) {
            targetFanSpeed = yCurvePoints[index+1];
        } else {
            targetFanSpeed = (((yCurvePoints[index + 1] - yCurvePoints[index]) * (temp - xCurvePoints[index])) / (xCurvePoints[index + 1] - xCurvePoints[index])) + yCurvePoints[index];
        }
    }
    QProcess process;
    QString input = nvFanSpeedSet;
    input.append(QString::number(targetFanSpeed));
    process.start(input);
    process.waitForFinished(-1);
}
void MainWindow::on_frequencySlider_valueChanged(int value)
{
    // Sets the input field value to slider value
    QString freqText = QString::number(value);
    ui->frequencySpinBox->setValue(value);
}

void MainWindow::on_frequencySpinBox_valueChanged(int arg1)
{
    ui->frequencySlider->setValue(arg1);
}

void MainWindow::on_newProfile_clicked()
{
    newProfile *newprof = new newProfile(this);
    newprof->setAttribute(Qt::WA_DeleteOnClose);
    connect(newprof, SIGNAL(destroyed(QObject*)), SLOT(on_newProfile_closed()));
    newprof->setModal(true);
    newprof->exec();
}

void MainWindow::on_powerLimSlider_valueChanged(int value)
{
    ui->powerLimSpinBox->setValue(value);
}
void MainWindow::on_powerLimSpinBox_valueChanged(int arg1)
{
    ui->powerLimSlider->setValue(arg1);
}
void MainWindow::on_memClkSlider_valueChanged(int value)
{
    ui->memClkSpinBox->setValue(value);
}
void MainWindow::on_memClkSpinBox_valueChanged(int arg1)
{
    ui->memClkSlider->setValue(arg1);
}
void MainWindow::on_voltageSlider_valueChanged(int value)
{
    ui->voltageSpinBox->setValue(value);
}
void MainWindow::on_voltageSpinBox_valueChanged(int arg1)
{
    ui->voltageSlider->setValue(arg1);
}
void MainWindow::on_fanSlider_valueChanged(int value)
{
    ui->fanSpinBox->setValue(value);
    fanUpdaterDisablerTimer->start(5000);
    fanUpdaterDisablerTimer->setSingleShot(true);
    disconnect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(fanSpeedUpdater()));
    connect(fanUpdaterDisablerTimer, SIGNAL(timeout()), this, SLOT(enableFanUpdater()));
}
void MainWindow::on_fanSpinBox_valueChanged(int arg1)
{
    ui->fanSlider->setValue(arg1);
}
void MainWindow::enableFanUpdater()
{
    connect(fanUpdateTimer, SIGNAL(timeout()), this, SLOT(fanSpeedUpdater()));
}
void MainWindow::on_applyButton_clicked()
{
    applyGPUSettings();
    saveProfileSettings();
    applyFanMode();
    setupMonitorTab();
    //ui->statusLabel->setText("Settings applied.");
    statusLabelResetTimer->start(7000);
    statusLabelResetTimer->setSingleShot(true);
    connect(statusLabelResetTimer, SIGNAL(timeout()), SLOT(resetStatusLabel()));
}
void MainWindow::on_editFanCurveButton_pressed()
{
    editProfile *editProf = new editProfile(this);
    editProf->setAttribute(Qt::WA_DeleteOnClose);
    connect(editProf, SIGNAL(destroyed(QObject*)), SLOT(on_editProfile_closed()));
    editProf->setModal(true);
    editProf->exec();

}
void MainWindow::on_editProfile_closed()
{
    qDebug() << "fancurve dialog closed";
    // Clear the existing curve points and load the new ones
    xCurvePoints.clear();
    yCurvePoints.clear();
    loadProfileSettings();
}

void MainWindow::on_fanModeComboBox_currentIndexChanged(int index)
{
    fanControlMode = index;
}
