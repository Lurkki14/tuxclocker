#pragma once

#include <QObject>
#include <QTimer>

#include "ReadableData.h"
#include "ReadableMasterObservable.h"

class ReadableMasterObservable;

class ReadableObservable : public QObject {
public:
    ReadableObservable(ReadableMasterObservable *masterObservable, QObject *parent = nullptr);
    void setInterval(std::chrono::milliseconds interval); // Set the emission interval
signals:
    void valueUpdated(tc_readable_result_t value);
    void intervalChanged(std::chrono::milliseconds interval);
private:
    Q_OBJECT
    
    QTimer *m_emitTimer;
    tc_readable_result_t m_latestValue;
};
