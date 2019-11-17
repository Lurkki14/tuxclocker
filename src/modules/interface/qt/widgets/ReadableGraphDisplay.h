#pragma once

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QtCharts/QChart>
#include <QDebug>

using namespace QtCharts;

class ReadableGraphDisplay : public QGraphicsView {
public:
    ReadableGraphDisplay(QWidget *parent = nullptr);
protected:
    bool event(QEvent *event);
private:
    QChart *m_chart;
};
