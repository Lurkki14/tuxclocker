#include "DragChartView.hpp"

#include <QDebug>
#include <QToolTip>
#include <QApplication>
#include <QValueAxis>
#include <QScreen>
#include <QWindow>

DragChartView::DragChartView(QWidget *parent) : QChartView(parent)
{
    chart()->installEventFilter(this);
    
    setRenderHint(QPainter::Antialiasing);

    m_toolTipLabel = new QLabel;
    m_toolTipLabel->setWindowFlag(Qt::ToolTip);

    m_dragCanStart = false;
    
    m_mouseInLimitArea = false;
    
    m_scatterPressed = false;

    m_leftLineFillerItem = new QGraphicsLineItem;
    m_rightLineFillerItem = new QGraphicsLineItem;
	m_leftLineFillerItem->setPen(fillerLinePen());
	m_rightLineFillerItem->setPen(fillerLinePen());

    chart()->scene()->addItem(m_leftLineFillerItem);
    chart()->scene()->addItem(m_rightLineFillerItem);
	chart()->legend()->setVisible(false);
    
    m_chartMargin = m_series.markerSize() / 2;
    
    // Resize axes by margin
    connect(&m_xAxis, &QValueAxis::rangeChanged, [=](qreal min, qreal max) {
        m_limitRect.setLeft(min);
        m_limitRect.setRight(max);
        
        m_limitRect.setTopLeft(QPointF(min, m_limitRect.top()));
        m_limitRect.setBottomRight(QPointF(max, m_limitRect.bottom()));
        
        if (chart()->rect().isNull()) {
            return;
        }
        // Convert m_chartMargin to chart value
        auto valueDelta = abs(chart()->mapToValue(QPointF(0, 0)).x() - chart()->mapToValue(QPointF(m_chartMargin, 0)).x());
        
        m_xAxis.blockSignals(true); // Don't go to an infinite loop
        m_xAxis.setRange(min - valueDelta, max + valueDelta);
        m_xAxis.blockSignals(false);
    });
    
    connect(&m_yAxis, &QValueAxis::rangeChanged, [=](qreal min, qreal max) {
        // Update limit rect
        m_limitRect.setBottom(min);
        m_limitRect.setTop(max);
        
        m_limitRect.setTopLeft(QPointF(m_limitRect.left(), max));
        m_limitRect.setBottomRight(QPointF(m_limitRect.right(), min));
        
        if (chart()->rect().isNull()) {
            return;
        }
        auto valueDelta = abs(chart()->mapToValue(QPointF(0, 0)).x() - chart()->mapToValue(QPointF(m_chartMargin, 0)).x());
        m_yAxis.blockSignals(true); // Don't go to an infinite loop
        m_yAxis.setRange(min - valueDelta, max + valueDelta);
        m_yAxis.blockSignals(false);
    });

    // Delete filler items when points are removed
    connect(&m_series, &QScatterSeries::pointRemoved, [=]() {
        chart()->scene()->removeItem(m_lineFillerItems.last());
        delete m_lineFillerItems.last();
        m_lineFillerItems.pop_back();
		drawFillerLines(&m_series);
    });
    
    // Add filler item when point is added
    connect(&m_series, &QScatterSeries::pointAdded, [=]() {
		if (m_series.pointsVector().length() < 2)
			return;
		
        auto item = new QGraphicsLineItem;
        item->setPen(fillerLinePen());
        m_lineFillerItems.append(item);
        chart()->scene()->addItem(item);
		drawFillerLines(&m_series);
    });
    
    connect(&m_series, &QScatterSeries::pointsReplaced, [=]() {
        // Delete filler items
        for (auto item : m_lineFillerItems) {
            delete item;
        }
        m_lineFillerItems.clear();
        // Create new ones
        for (int i = 0; i < m_series.pointsVector().length(); i++) {
            auto item = new QGraphicsLineItem;
            item->setPen(fillerLinePen());
            m_lineFillerItems.append(item);
            chart()->scene()->addItem(item);
        }
        drawFillerLines(&m_series);
    });
    
    connect(&m_series, &QScatterSeries::pressed, [=](QPointF point) {
       m_dragCanStart = true;
       m_latestScatterPoint = point;
       m_scatterPressed = true;
    });
    
    connect(&m_series, &QScatterSeries::clicked, [=](const QPointF point) {
        m_scatterPressed = false;
        if (m_dragActive) {
            m_dragActive = false;
            emit dragEnded(point);
        }
        else {
            // Just a click, delete point
            m_series.remove(point);
        }
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
    
    // Set cursor to indicate dragging
    connect(this, &DragChartView::dragStarted, [=]() {
        setCursor(Qt::ClosedHandCursor);
    });
    
    connect(this, &DragChartView::dragEnded, [=]() {
        if (m_mouseInLimitArea) {
            setCursor(Qt::CrossCursor);
        }
        else {
            setCursor(Qt::ArrowCursor);
        }
    });
    
    connect(this, &DragChartView::limitAreaEntered, [=]() {
        setCursor(Qt::CrossCursor);
    });
    
    connect(this, &DragChartView::limitAreaExited, [=]() {
        if (cursor().shape() != Qt::ClosedHandCursor) {
            setCursor(Qt::ArrowCursor);
        }
    });
}

void DragChartView::setVector(const QVector <QPointF> vector) {
    m_series.replace(vector);
}

void DragChartView::setRange(qreal xmin, qreal xmax, qreal ymin, qreal ymax) {
	m_series.clear();
	m_xAxis.setRange(xmin, xmax);
	m_yAxis.setRange(ymin, ymax);
}

bool DragChartView::event(QEvent *event) {
    //qDebug() << event->type();

    if (event->type() == QEvent::Resize || event->type() == QEvent::UpdateLater) {
        // Chart has a geometry when this is true
        drawFillerLines(&m_series);
    }
    
    if (event->type() == QEvent::Leave && !m_dragActive) {
        // Set to normal cursor
        setCursor(Qt::ArrowCursor);
    }

    return QChartView::event(event);
}

void DragChartView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }

    QChartView::mousePressEvent(event);
}

void DragChartView::mouseMoveEvent(QMouseEvent *event) {    
    if (m_limitRect.contains(chart()->mapToValue(event->pos())) && !m_mouseInLimitArea) {
        m_mouseInLimitArea = true;
        emit limitAreaEntered();
    }
    
    if (!m_limitRect.contains(chart()->mapToValue(event->pos())) && m_mouseInLimitArea) {
        m_mouseInLimitArea = false;
        emit limitAreaExited();
    }
    
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance() || !m_dragCanStart) {
        return;
    }

    // Start drag
   emit dragStarted(m_dragStartPosition);
   
    m_dragActive = true;

    if (m_toolTipLabel->isHidden()) {
        m_toolTipLabel->show();
    }

    m_toolTipLabel->setText(QString("%1, %2")
		.arg(QString::number(m_latestScatterPoint.x()),
		QString::number(m_latestScatterPoint.y())));
	
    // FIXME : doesn't work properly when screen is switched(?)
    m_toolTipLabel->move(event->screenPos().toPoint() + toolTipOffset(this, event->windowPos().toPoint()));
    
    // Don't move point out of bounds
    if (m_limitRect.contains(chart()->mapToValue(event->pos()))) {
        replaceMovedPoint(m_latestScatterPoint, chart()->mapToValue(event->pos(), &m_series));
    }
    else {
        QPointF point(chart()->mapToValue(event->pos()));
        // Set the point value to the constraint where it exceeds it
        if (chart()->mapToValue(event->pos()).x() > m_limitRect.right()) {
            point.setX(m_limitRect.right());
        }
        if (chart()->mapToValue(event->pos()).x() < m_limitRect.left()) {
            point.setX(m_limitRect.left());
        }
        if (chart()->mapToValue(event->pos()).y() > m_limitRect.top()) {
            point.setY(m_limitRect.top());
        }
        if (chart()->mapToValue(event->pos()).y() < m_limitRect.bottom()) {
            point.setY(m_limitRect.bottom());
        }
        replaceMovedPoint(m_latestScatterPoint, point);
    }

    drawFillerLines(&m_series);
}

void DragChartView::mouseReleaseEvent(QMouseEvent *event) {
    m_dragCanStart = false;

    if (!m_scatterPressed && 
			m_limitRect.contains(chart()->mapToValue(event->pos()))) {
		// Add a new point to series
		m_series.append(chart()->mapToValue(event->pos()));
		drawFillerLines(&m_series);
    }
    
    m_toolTipLabel->hide();
    QChartView::mouseReleaseEvent(event);
}

void DragChartView::wheelEvent(QWheelEvent *event) {
    qreal factor = event->angleDelta().y() > 0 ? 0.5 : 2.0;

    //chart()->zoom(factor);
    event->accept();

    drawFillerLines(&m_series);

    QChartView::wheelEvent(event);
}

void DragChartView::replaceMovedPoint(const QPointF old, const QPointF new_) {
    m_series.replace(old, new_);

    m_latestScatterPoint = new_;
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
    // TODO: line isn't drawn between points whose x is the same
	// Sort points by ascending x
    QVector <QPointF> sorted = sortPointFByAscendingX(series->pointsVector());

    if (sorted.isEmpty()) {
        return;
    }

    for (int i = 0; i < sorted.length() - 1; i++) {
        m_lineFillerItems[i]->setLine(QLineF(chart()->mapToPosition(sorted[i]),
			chart()->mapToPosition(sorted[i + 1])));
    }

    m_leftLineFillerItem->setLine(
		QLineF(chart()->mapToPosition(QPointF(
			m_xAxis.min(), sorted[0].y())),
		chart()->mapToPosition(sorted[0])));

    m_rightLineFillerItem->setLine(
		QLineF(chart()->mapToPosition(sorted.last()),
        chart()->mapToPosition(QPointF(m_xAxis.max(), sorted.last().y()))));

    chart()->update();
}

bool DragChartView::eventFilter(QObject *obj, QEvent *event) {
    static bool resized = false;
    
    if (obj == chart() &&  event->type() == QEvent::WindowActivate && !resized) {
        resized = true;
        emit m_xAxis.rangeChanged(m_xAxis.min(), m_xAxis.max());
        emit m_yAxis.rangeChanged(m_yAxis.min(), m_yAxis.max());
    }
    return QObject::eventFilter(obj, event);
}

QPoint DragChartView::toolTipOffset(QWidget *widget, const QPoint windowCursorPos) {
    QRect screenRect = widget->window()->windowHandle()->screen()->geometry();
    
    if (screenRect.width() > screenRect.height()) {
        // Use x offset for screens that are wider than high
        // Set the offset to the side that has more space
        int xOffset = (windowCursorPos.x() > widget->window()->rect().width() / 2) ? -qRound(toolTipMargin() * screenRect.width()) : qRound(toolTipMargin() * screenRect.width());
        return QPoint(xOffset, 0);
    }
    int yOffset = (windowCursorPos.y() > widget->window()->rect().height() / 2) ? -qRound(toolTipMargin() * screenRect.height()) :qRound(toolTipMargin() * screenRect.height());
    
    return QPoint(0, yOffset);
}
