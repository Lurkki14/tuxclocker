#include "DragChartView.h"

#include <QDebug>
#include <QToolTip>
#include <QApplication>
#include <QValueAxis>

DragChartView::DragChartView(QWidget *parent) : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);

    m_toolTipLabel = new QLabel;
    m_toolTipLabel->setWindowFlag(Qt::ToolTip);

    m_dragCanStart = false;

    m_leftLineFillerItem = new QGraphicsLineItem;
    m_rightLineFillerItem = new QGraphicsLineItem;

    chart()->scene()->addItem(m_leftLineFillerItem);
    chart()->scene()->addItem(m_rightLineFillerItem);

    for (int i = 0; i < 5; i++) {
        m_series.append(i * 5, i * 5);

        auto item = new QGraphicsLineItem;
        item->setPen(QPen(QBrush(QColor(Qt::blue)), 3));

        m_lineFillerItems.append(item);
        chart()->scene()->addItem(item);
    }

    connect(&m_series, &QScatterSeries::pressed, [=](QPointF point) {
       qDebug() << "click at" << point;
       m_dragCanStart = true;
       m_latestScatterPoint = point;
    });


    chart()->addSeries(&m_series);
    
    chart()->addAxis(&m_xAxis, Qt::AlignBottom);
    chart()->addAxis(&m_yAxis, Qt::AlignLeft);
    
    m_series.attachAxis(&m_xAxis);
    m_series.attachAxis(&m_yAxis);   
    
    m_xAxis.setRange(0, 25);
    m_yAxis.setRange(0, 25);

    chart()->setBackgroundRoundness(0);
    
    // Set theme colors
    chart()->setBackgroundBrush(QBrush(QPalette().color(QPalette::Background)));
    
    chart()->legend()->setLabelColor(QPalette().color(QPalette::Text));
    
    m_yAxis.setLabelsColor(QPalette().color(QPalette::Text));
    m_xAxis.setLabelsColor(QPalette().color(QPalette::Text));
    
    m_yAxis.setTitleBrush(QBrush(QPalette().color(QPalette::Text)));
    m_xAxis.setTitleBrush(QBrush(QPalette().color(QPalette::Text)));
    
    //setCursor(Qt::CrossCursor);
}

bool DragChartView::event(QEvent *event) {
    //qDebug() << event->type();

    if (event->type() == QEvent::Resize || event->type() == QEvent::UpdateLater) {
        // Chart has a geometry when this is true
        //qDebug() << chart()->mapToPosition(QPointF(5, 5), &m_series);

        //QLineF line(chart()->mapToPosition(m_series.pointsVector()[0]), chart()->mapToPosition(m_series.pointsVector()[1]));

        drawFillerLines(&m_series);
    }

    return QChartView::event(event);
}

void DragChartView::mousePressEvent(QMouseEvent *event) {
    qDebug() << event->type();

    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }

    QChartView::mousePressEvent(event);
}

void DragChartView::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance() || !m_dragCanStart) {
        return;
    }
    // Start drag
    // Set cursor to indicate dragging
   // setCursor(Qt::ClosedHandCursor);
    
    m_dragActive = true;

    //QToolTip::showText(mapToGlobal(event->pos()), QString("%1, %2").arg(QString::number(m_latestScatterPoint.x()), QString::number(m_latestScatterPoint.y())));

    if (m_toolTipLabel->isHidden()) {
        m_toolTipLabel->show();
    }

    m_toolTipLabel->setText(QString("%1, %2").arg(QString::number(m_latestScatterPoint.x()), QString::number(m_latestScatterPoint.y())));
    m_toolTipLabel->move(event->screenPos().toPoint());
    replaceMovedPoint(m_latestScatterPoint, chart()->mapToValue(event->pos(), &m_series));

    drawFillerLines(&m_series);
}

void DragChartView::mouseReleaseEvent(QMouseEvent *event) {
    qDebug() << event->type();

    m_dragCanStart = false;

    qDebug() << chart()->mapToValue(event->pos(), &m_series);

    if (m_dragActive) {
        //replaceMovedPoint(m_latestScatterPoint, chart()->mapToValue(event->pos(), &m_series));
    }

    //QToolTip::hideText();
    m_toolTipLabel->hide();
    QChartView::mouseReleaseEvent(event);
}

void DragChartView::wheelEvent(QWheelEvent *event) {
    qDebug() << event->angleDelta();
    qreal factor = event->angleDelta().y() > 0 ? 0.5 : 2.0;

    //chart()->zoom(factor);
    event->accept();

    drawFillerLines(&m_series);

    QChartView::wheelEvent(event);
}

void DragChartView::replaceMovedPoint(const QPointF old, const QPointF new_) {
    m_series.replace(old, new_);

    m_latestScatterPoint = new_;

    //m_dragActive = false;
}

QVector <QPointF> DragChartView::sortPointFByAscendingX(const QVector<QPointF> points) {
    QVector <qreal> ascendingX;
    QVector <qreal> originalY;

    for (QPointF point : points) {
        ascendingX.append(point.x());
        originalY.append(point.y());
    }

    std::sort(ascendingX.begin(), ascendingX.end());

    QVector <QPointF> sorted;

    // Find the original y values for x values
    for (qreal x : ascendingX) {
        for (QPointF point : points) {
            if (qFuzzyCompare(x, point.x())) {
                sorted.append(QPointF(x, point.y()));
                break;
            }
        }
    }

    return sorted;
}

void DragChartView::drawFillerLines(QScatterSeries *series) {
    // Sort points by ascending x
    QVector <QPointF> sorted = sortPointFByAscendingX(series->pointsVector());

    if (sorted.isEmpty()) {
        return;
    }

    for (int i = 0; i < sorted.length() - 1; i++) {
        m_lineFillerItems[i]->setLine(QLineF(chart()->mapToPosition(sorted[i]),
                                             chart()->mapToPosition(sorted[i + 1])));
    }

    m_leftLineFillerItem->setLine(QLineF(chart()->mapToPosition(QPointF(m_xAxis.min(), sorted[0].y())),
                                         chart()->mapToPosition(sorted[0])));

    m_rightLineFillerItem->setLine(QLineF(chart()->mapToPosition(sorted.last()),
                                          chart()->mapToPosition(QPointF(m_xAxis.max(), sorted.last().y()))));

    qDebug() << chart()->mapToValue(m_rightLineFillerItem->line().p2());

    chart()->update();
}
