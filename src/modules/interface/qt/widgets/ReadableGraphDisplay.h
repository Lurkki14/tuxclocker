#pragma once

#include <QChartView>
#include <QGraphicsItem>
#include <QtCharts/QChart>
#include <QLineSeries>
#include <QCategoryAxis>
#include <QDebug>

// Graph view for updating readables. QVariantLists convertible to  ReadableData can be dropped on this widget.

using namespace QtCharts;

class ReadableGraphDisplay : public QChartView {
public:
    ReadableGraphDisplay(QWidget *parent = nullptr);
protected:
    bool event(QEvent *event);
    
    void resizeEvent(QResizeEvent *event) {
        fitInView(sceneRect());
    }
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    Q_OBJECT
    
    QChart *m_chart;
    
    QValueAxis xAxis, yAxis;
    
    int x;
    
    QLineSeries series;
};
