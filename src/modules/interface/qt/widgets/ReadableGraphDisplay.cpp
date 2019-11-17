#include "ReadableGraphDisplay.h"

ReadableGraphDisplay::ReadableGraphDisplay(QWidget *parent) : QGraphicsView(new QGraphicsScene, parent) {
    m_chart = new QChart;
    m_chart->createDefaultAxes();
    
    scene()->addItem(m_chart);
}

bool ReadableGraphDisplay::event(QEvent *event) {
    if (event->type() == QEvent::Leave) {
        qDebug() << "leave";
        return true;
    }
    return QAbstractScrollArea::event(event);
}
