#pragma once

#include <QChartView>
#include <QGraphicsItem>
#include <QtCharts/QChart>
#include <QLegendMarker>
#include <QLineSeries>
#include <QCategoryAxis>
#include <QDebug>

#include <ReadableMasterObservable.h>
#include <ReadableObservableManager.h>

// Graph view for updating readables. QVariantLists convertible to  ReadableData can be dropped on this widget.

using namespace QtCharts;

class ReadableGraphDisplay : public QChartView {
public:
    ReadableGraphDisplay(ReadableObservableManager *manager, QWidget *parent = nullptr);
public slots:
    //void updateBounds(qreal y);
protected:
    bool event(QEvent *event);
    
    void resizeEvent(QResizeEvent *event) {
        fitInView(sceneRect());
    }
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    Q_OBJECT
    
    // Struct for queueing values
    struct ValueQueue {
        QLineSeries *series;
        tc_readable_result_t res;
    };
    double yAxisMargin; // Margin as a percentage (0-1) of the plot between min/max values and x axis min/max
    
    ReadableObservableManager *m_observableManager; // For creating observables
    
    QChart *m_chart;
    
    QValueAxis xAxis, yAxis;
    
    int x;
    
    QVector <QLineSeries*> m_seriesList;
    
    ReadableObservable *m_referenceObservable; // Observable that controls when queued values are added
    
    QVector <ValueQueue> m_queuedValues;
    
    bool addToSeries(QLineSeries *series, tc_readable_result_t res); // Add a value to series. Returns res.valid
    
    void addQueuedValues();
    
    void adjustYAxisBounds(QLineSeries *series);
};
