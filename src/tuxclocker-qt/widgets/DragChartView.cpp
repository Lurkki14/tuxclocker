#include "DragChartView.hpp"

#include <QDebug>
#include <QToolTip>
#include <QApplication>
#include <QValueAxis>
#include <QPainterPath>
#include <QScreen>
#include <QWindow>

DragChartView::DragChartView(QWidget *parent) : QChartView(parent) {
	chart()->installEventFilter(this);

	setRenderHint(QPainter::Antialiasing);

	m_toolTipLabel = new QLabel{this};
	m_toolTipLabel->setWindowFlag(Qt::ToolTip);

	m_dragCanStart = false;

	m_mouseInLimitArea = false;

	m_scatterPressed = false;

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
		auto valueDelta = abs(chart()->mapToValue(QPointF(0, 0)).x() -
				      chart()->mapToValue(QPointF(m_chartMargin, 0)).x());

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
		auto valueDelta = abs(chart()->mapToValue(QPointF(0, 0)).x() -
				      chart()->mapToValue(QPointF(m_chartMargin, 0)).x());
		m_yAxis.blockSignals(true); // Don't go to an infinite loop
		m_yAxis.setRange(min - valueDelta, max + valueDelta);
		m_yAxis.blockSignals(false);
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
		} else {
			// Just a click, delete point
			m_series.remove(point);
		}
	});

	connect(&m_series, &QScatterSeries::hovered, [=](auto point, bool state) {
		if (m_dragActive)
			return;
		// Show value when hovered
		m_toolTipLabel->setText(labelText(point));
		updateToolTipPos(point);
		m_showToolTip = state;
	});

	connect(&m_series, &QScatterSeries::pointAdded,
	    [=](auto) { emit pointsChanged(m_series.pointsVector()); });

	connect(&m_series, &QScatterSeries::pointRemoved,
	    [=](auto) { emit pointsChanged(m_series.pointsVector()); });

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
	connect(this, &DragChartView::dragStarted, [=]() { setCursor(Qt::ClosedHandCursor); });

	connect(this, &DragChartView::dragEnded, [=]() {
		if (m_mouseInLimitArea) {
			setCursor(Qt::CrossCursor);
		} else {
			setCursor(Qt::ArrowCursor);
		}
	});

	connect(this, &DragChartView::limitAreaEntered, [=]() { setCursor(Qt::CrossCursor); });

	connect(this, &DragChartView::limitAreaExited, [=]() {
		if (cursor().shape() != Qt::ClosedHandCursor) {
			setCursor(Qt::ArrowCursor);
		}
	});
}

QString DragChartView::labelText(const QPointF &point) {
	return QString{"%1, %2"}.arg(point.x(), 0, 'f', 2).arg(point.y(), 0, 'f', 2);
}

QPoint DragChartView::toolTipOffset1(const QPointF &chartPos) {
	m_toolTipLabel->adjustSize();
	auto tooltipWidth = m_toolTipLabel->geometry().width();
	auto tooltipHeight = m_toolTipLabel->geometry().height();

	auto offset = m_series.pen().width() * 4;
	// Position relative to point
	auto dy = m_yAxis.max() - m_yAxis.min();
	auto yCenter = m_yAxis.max() - (dy / 2);
	auto placeTop = chartPos.y() < yCenter;
	// y increases towards the bottom
	auto yOffset = placeTop ? (-offset - tooltipHeight) : offset;

	auto dx = m_xAxis.max() - m_xAxis.min();
	auto xCenter = m_xAxis.max() - (dx / 2);
	auto placeRight = chartPos.x() < xCenter;
	auto xOffset = placeRight ? offset : (-offset - tooltipWidth);

	return QPoint{xOffset, yOffset};
}

void DragChartView::setVector(const QVector<QPointF> vector) { m_series.replace(vector); }

void DragChartView::setRange(qreal xmin, qreal xmax, qreal ymin, qreal ymax) {
	m_series.clear();
	m_xAxis.setRange(xmin, xmax);
	m_yAxis.setRange(ymin, ymax);
}

bool DragChartView::event(QEvent *event) {
	// qDebug() << event->type();

	if (event->type() == QEvent::Resize || event->type() == QEvent::UpdateLater) {
		// Chart has a geometry when this is true
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
		QChartView::mouseMoveEvent(event);
		return;
	}
	if ((event->pos() - m_dragStartPosition).manhattanLength() <
		QApplication::startDragDistance() ||
	    !m_dragCanStart) {
		QChartView::mouseMoveEvent(event);
		return;
	}

	// Start drag
	emit dragStarted(m_dragStartPosition);

	m_dragActive = true;

	if (m_toolTipLabel->isHidden()) {
		m_toolTipLabel->show();
	}

	// Debounce label updates (limit frequency)
	// TODO: time-based might be better?
	static uint moveEventCount = 0;
	if (moveEventCount % 5 == 0) {
		auto text = QString("%1, %2")
				.arg(m_latestScatterPoint.x(), 0, 'f', 2)
				.arg(m_latestScatterPoint.y(), 0, 'f', 2);
		m_toolTipLabel->setText(text);
		// Don't cut the label text off
		// This is only actually needed on the first try (why though?)
		m_toolTipLabel->adjustSize();

		// FIXME : doesn't work properly when screen is switched(?)
		m_toolTipLabel->move(event->screenPos().toPoint() +
				     toolTipOffset(this, event->windowPos().toPoint()));
	}
	moveEventCount++;

	// Don't move point out of bounds
	if (m_limitRect.contains(chart()->mapToValue(event->pos()))) {
		replaceMovedPoint(
		    m_latestScatterPoint, chart()->mapToValue(event->pos(), &m_series));
	} else {
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
}

void DragChartView::mouseReleaseEvent(QMouseEvent *event) {
	m_dragCanStart = false;

	if (!m_scatterPressed && m_limitRect.contains(chart()->mapToValue(event->pos()))) {
		// Add a new point to series
		m_series.append(chart()->mapToValue(event->pos()));
	}

	m_toolTipLabel->hide();
	QChartView::mouseReleaseEvent(event);
}

void DragChartView::updateToolTipPos(const QPointF &chartPos) {
	auto realPos = mapToGlobal(chart()->mapToPosition(chartPos).toPoint());
	m_toolTipPos = realPos + toolTipOffset1(chartPos);
}

void DragChartView::zoomX(qreal factor) {
	auto newMax = m_xAxis.max() * factor;
	// Don't zoom when points would be moved out of bounds
	for (auto &point : m_series.points()) {
		if (point.x() > newMax)
			// TODO: indicate in the GUI when this happens
			return;
	}

	// TODO: allow moving to negative x if that ever makes sense
	m_xAxis.setMax(newMax);
	m_xAxis.setMin(0);
}

void DragChartView::wheelEvent(QWheelEvent *event) {
	qreal factor = event->angleDelta().y() > 0 ? 0.5 : 2.0;

	zoomX(factor);
	event->accept();

	QChartView::wheelEvent(event);
}

void DragChartView::replaceMovedPoint(const QPointF old, const QPointF new_) {
	m_series.replace(old, new_);

	m_latestScatterPoint = new_;
}

QVector<QPointF> DragChartView::sortPointFByAscendingX(const QVector<QPointF> points) {
	QVector<qreal> ascendingX;
	QVector<qreal> originalY;

	for (QPointF point : points) {
		ascendingX.append(point.x());
		originalY.append(point.y());
	}

	std::sort(ascendingX.begin(), ascendingX.end());

	QVector<QPointF> sorted;

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

void DragChartView::drawForeground(QPainter *painter, const QRectF &rect) {
	if (m_showToolTip && !m_dragActive) {
		m_toolTipLabel->move(m_toolTipPos);
		m_toolTipLabel->show();
	} else if (!m_dragActive)
		m_toolTipLabel->hide();

	auto points = sortPointFByAscendingX(m_series.pointsVector());

	if (points.empty()) {
		QChartView::drawForeground(painter, rect);
		return;
	}

	// TODO: filler line lingers around even without items, until the chart is updated
	// Filler line between left edge and first point
	auto first = chart()->mapToPosition(points.first());
	auto start = chart()->mapToPosition(QPointF{m_xAxis.min(), points.first().y()});

	QPainterPath path;
	path.moveTo(start);
	path.lineTo(first);

	// Between points
	if (points.length() > 1) {
		for (auto &point : points) {
			auto realPos = chart()->mapToPosition(point);
			path.lineTo(realPos);
			path.moveTo(realPos);
		}
	}

	// Last point -> right edge
	auto last = chart()->mapToPosition(points.last());
	auto end = chart()->mapToPosition(QPointF{m_xAxis.max(), points.last().y()});

	path.moveTo(last);
	path.lineTo(end);

	painter->setPen(fillerLinePen());
	painter->drawPath(path);

	QChartView::drawForeground(painter, rect);
}

bool DragChartView::eventFilter(QObject *obj, QEvent *event) {
	static bool resized = false;

	if (obj == chart() && event->type() == QEvent::WindowActivate && !resized) {
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
		int xOffset = (windowCursorPos.x() > widget->window()->rect().width() / 2)
				  ? -qRound(toolTipMargin() * screenRect.width())
				  : qRound(toolTipMargin() * screenRect.width());
		return QPoint(xOffset, 0);
	}
	int yOffset = (windowCursorPos.y() > widget->window()->rect().height() / 2)
			  ? -qRound(toolTipMargin() * screenRect.height())
			  : qRound(toolTipMargin() * screenRect.height());

	return QPoint(0, yOffset);
}
