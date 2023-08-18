#pragma once

#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QLabel>
#include <QValueAxis>

using namespace QtCharts;

class DragChartView : public QChartView {
public:
	DragChartView(QWidget *parent = nullptr);
	QValueAxis &xAxis() { return m_xAxis; }
	QValueAxis &yAxis() { return m_yAxis; }

	void setVector(const QVector<QPointF> vector);
	QVector<QPointF> vector() { return m_series.pointsVector(); }
	// Clear the points and set new range
	void setRange(qreal xmin, qreal xmax, qreal ymin, qreal ymax);
protected:
	bool event(QEvent *);
	void drawForeground(QPainter *, const QRectF &) override;
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void wheelEvent(QWheelEvent *);

	bool eventFilter(QObject *obj, QEvent *event); // Get notified when chart has geometry
signals:
	void dragStarted(const QPointF point);
	void dragEnded(const QPointF point);

	void limitAreaEntered();
	void limitAreaExited();
private:
	Q_OBJECT

	static QPoint toolTipOffset(QWidget *widget,
	    const QPoint windowCursorPos); // Get tooltip offset based on current screen
	static double toolTipMargin() { return 0.02; }

	QColor fillerLineColor() { return QPalette().color(QPalette::Highlight); }
	QPen fillerLinePen() { return QPen(fillerLineColor(), 3); }

	qreal m_chartMargin; // Margin in pixels between a scatter point and axis edge. Add this
			     // value on either edge of the axes in order for chatter points to be
			     // unable to go partially out of the viewport.
	QRectF m_limitRect;  // Scatter points can't be moved outside this rect
	bool m_mouseInLimitArea;

	bool m_scatterPressed; // For detecting if mouse release should delete or add a new point

	QVector<QPointF> sortPointFByAscendingX(const QVector<QPointF> points);
	void zoomX(qreal);

	QLabel *m_toolTipLabel;
	bool m_showToolTip;
	QPoint m_toolTipPos; // Global position
	void updateToolTipPos(const QPointF &chartPos);
	// Place tooltip towards center of the chart
	QPoint toolTipOffset1(const QPointF &chartPos);
	static QString labelText(const QPointF &chartPoint);

	QScatterSeries m_series;
	QPoint m_dragStartPosition;
	bool m_dragCanStart; // Was a point clicked and not released before drag should start
	bool m_dragActive;
	QPointF m_latestScatterPoint;
	void replaceMovedPoint(const QPointF old, const QPointF new_);
	QValueAxis m_xAxis, m_yAxis;
};
