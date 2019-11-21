#include "ReadableGraphDisplay.h"

#include <ReadableData.h>
#include <ReadableMasterObservable.h>

#include <QDragEnterEvent>
#include <QMimeData>

ReadableGraphDisplay::ReadableGraphDisplay(QWidget *parent) : QChartView(parent) {
    // FIXME: drops are only accepted only on a small area on the widget
    setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing);
    
    m_chart = new QChart;
    
    setChart(m_chart);
    
    m_chart->addSeries(&series);
    
    m_chart->addAxis(&xAxis, Qt::AlignBottom);
    m_chart->addAxis(&yAxis, Qt::AlignLeft);
    
    series.attachAxis(&xAxis);
    series.attachAxis(&yAxis);
    
    yAxis.setRange(0, 70000);
    xAxis.setRange(0, 20);
    
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
    //QVariantList itemDataList;
    QByteArray b_data = event->mimeData()->data(ReadableData::mimeType());
    
    qDebug() << b_data;
    
    // Convert mime data to QVariantList
    
    QDataStream stream(&b_data, QIODevice::ReadOnly);
    
    QVariant v;
    stream >> v;
    
    qDebug() << v;
    
    if (!v.canConvert<ReadableData>()) {
        qDebug() << "can't convert";
    }
    
    ReadableData d = qvariant_cast<ReadableData>(v);
    
    qDebug() << d.value().data.uint_value;
    
   /* if (itemDataList.size() < 1) {
        return;
    }*/
    
    ReadableMasterObservable *mo = new ReadableMasterObservable(&d);
    
    connect(mo->createObservable(std::chrono::milliseconds(2000)), &ReadableObservable::valueUpdated, [=](tc_readable_result_t res) {
        if (!res.valid) {
            return;
        }
        
        qDebug() << res.data.uint_value << res.valid;
        
        x++;
        series.append(x, res.data.uint_value);
    });
}
