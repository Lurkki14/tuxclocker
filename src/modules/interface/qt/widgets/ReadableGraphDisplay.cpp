#include "ReadableGraphDisplay.h"

#include <ReadableData.h>

#include <QDragEnterEvent>
#include <QMimeData>

ReadableGraphDisplay::ReadableGraphDisplay(ReadableObservableManager *manager, QWidget *parent) : QChartView(parent) {
    m_observableManager = manager;
    
    setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing);
    
    m_chart = new QChart;
    m_chart->setBackgroundRoundness(0);
    m_chart->setBackgroundBrush(QBrush(QPalette().color(QPalette::Background)));
    
    m_chart->legend()->setLabelColor(QPalette().color(QPalette::Text));
    
    m_chart->setAcceptDrops(true);
    
    setChart(m_chart);
    
    m_referenceObservable = nullptr;
    
    m_chart->addAxis(&xAxis, Qt::AlignBottom);
    m_chart->addAxis(&yAxis, Qt::AlignLeft);
    
    yAxis.setLabelsColor(QPalette().color(QPalette::Text));
    xAxis.setLabelsColor(QPalette().color(QPalette::Text));
    
    yAxis.setRange(0, 70000);
    xAxis.setRange(-20, 0);
    
    yAxisMargin = 0.1;
    
    x = 0;
}

bool ReadableGraphDisplay::event(QEvent *event) {
    if (event->type() == QEvent::Leave) {
        return true;
    }
    return QAbstractScrollArea::event(event);
}

void ReadableGraphDisplay::dragEnterEvent(QDragEnterEvent *event) {
    qDebug() << event->mimeData()->formats();
    
    if (event->mimeData()->hasFormat(ReadableData::mimeType())) {
        event->accept();
    }
}

void ReadableGraphDisplay::dropEvent(QDropEvent *event) {
    QByteArray b_data = event->mimeData()->data(ReadableData::mimeType());
    
    // Convert mime data to QVariantList
    QDataStream stream(&b_data, QIODevice::ReadOnly);
    
    QVariant v;
    stream >> v;
    
    if (!v.canConvert<ReadableData>()) {
        qDebug() << "can't convert";
        return;
    }
    
    ReadableData d = qvariant_cast<ReadableData>(v);
    
    qDebug() << d.value().data.uint_value;
    
   /* if (itemDataList.size() < 1) {
        return;
    }*/
    
    QLineSeries *series = new QLineSeries(this);
    series->setName(d.name());
    
    m_chart->addSeries(series);
    
    series->attachAxis(&xAxis);
    series->attachAxis(&yAxis);
    
    m_seriesList.append(series);
    
    if (!m_referenceObservable) {
        m_referenceObservable = m_observableManager->observable(&d, std::chrono::milliseconds(2000));
        
        connect(m_referenceObservable, &ReadableObservable::valueUpdated, [=](tc_readable_result_t res) {
            addToSeries(series, res);
            addQueuedValues();
        });
    }
    
    connect(m_observableManager->observable(&d, std::chrono::milliseconds(2000)), &ReadableObservable::valueUpdated, [=](tc_readable_result_t res) {
        ValueQueue vq = {
            .series = series,
            .res = res
        };
        m_queuedValues.append(vq);
    });
    
    connect(series, &QLineSeries::pointAdded, [=]() {
        adjustYAxisBounds(series);
    });
    
    for (QLegendMarker *marker : m_chart->legend()->markers()) {
        connect(marker, &QLegendMarker::clicked, []() {
            qDebug() << "marker clicked";
        });
    }
}

bool ReadableGraphDisplay::addToSeries(QLineSeries *series, tc_readable_result_t res) {
    if (!res.valid) {
        return false;
    }
    
    qreal value;
    
    switch (res.data.data_type) {
        case TC_TYPE_INT:
            //series->append(x, res.data.int_value);
            value = res.data.int_value;
            break;
        case TC_TYPE_UINT:
            value = res.data.uint_value;
            break;
        case TC_TYPE_DOUBLE:
            value = res.data.double_value;
            break;
        default:
            return false;
    }
    
    // Shift the series by -<update period> in the x axis
    QVector <QPointF> points = series->pointsVector();
    
    for (int i = 0; i < points.length(); i++) {
        points[i].setX(points[i].x() - 2);
    }
    
    if (points.length() > 20) {
        points.pop_front();
    }
    
    series->replace(points);
    
    series->append(0, value);
    
    return true;
}

void ReadableGraphDisplay::addQueuedValues() {
    for (ValueQueue vq : m_queuedValues) {
        addToSeries(vq.series, vq.res);
    }
    
    m_queuedValues.clear();
}

void ReadableGraphDisplay::adjustYAxisBounds(QLineSeries *series) {
    QVector <qreal> yVector;
    
    for (QLineSeries *series : m_seriesList) {
        for (QPointF point : series->points()) {
            yVector.append(point.y());
        }
    }
    
    qreal yMax = *std::max_element(yVector.begin(), yVector.end());
    qreal yMin = *std::min_element(yVector.begin(), yVector.end());
    
    if (qFuzzyCompare(yMin, yMax)) {
        // The graph is a straight line
        yAxis.setMax(yMax + (yMax * yAxisMargin));
        yAxis.setMin(yMax - (yMax * yAxisMargin));
    }
    else {
        yAxis.setMax(yMax + abs(yMax - yMin) * yAxisMargin);
        yAxis.setMin(yMin - abs(yMax - yMin) * yAxisMargin);
    }
}
