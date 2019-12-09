#pragma once

#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QLabel>
#include <QValueAxis>

using namespace QtCharts;

class DragChartView : public QChartView {
public:
    DragChartView(QWidget *parent = nullptr);
    QValueAxis *xAxis() {return &m_xAxis;}
    QValueAxis *yAxis() {return &m_yAxis;}
protected:
    bool event(QEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
/*signals:
    void dragStarted(const QPointF point);
    void dragEnded(const QPointF point);*/
private:
    QVector <QPointF> sortPointFByAscendingX(const QVector <QPointF> points);
    void drawFillerLines(QScatterSeries *series);
    QLabel *m_toolTipLabel;
    QScatterSeries m_series;
    QPoint m_dragStartPosition;
    bool m_dragCanStart; // Was a point clicked and not released before drag should start
    bool m_dragActive;
    QPointF m_latestScatterPoint;
    void replaceMovedPoint(const QPointF old, const QPointF new_);
    QVector <QGraphicsLineItem*> m_lineFillerItems; // Filler lines between points whose amount is n - 1 for nonzero n
    QGraphicsLineItem *m_leftLineFillerItem;
    QGraphicsLineItem *m_rightLineFillerItem;
    QValueAxis m_xAxis, m_yAxis;
};
