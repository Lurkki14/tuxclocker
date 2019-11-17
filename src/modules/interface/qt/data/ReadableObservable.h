#pragma once

#include <QObject>
#include <QTimer>

#include "ReadableData.h"

class ReadableObservable : public QObject {
public:
    ReadableObservable(ReadableData *data, QObject *parent = nullptr);
    void setInterval(std::chrono::milliseconds msecs); // Set the emission interval
signals:
    void valueUpdated(tc_readable_result_t value);
private:
    Q_OBJECT
    
    ReadableData *m_readableData;
    QTimer *m_emitTimer;
    tc_readable_result_t m_latestValue;
};
