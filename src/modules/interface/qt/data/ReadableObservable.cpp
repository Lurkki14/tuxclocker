#include "ReadableObservable.h"

ReadableObservable::ReadableObservable(ReadableMasterObservable *masterObservable, QObject *parent) : QObject(parent) {
    connect(masterObservable, &ReadableMasterObservable::valueUpdated, [=](tc_readable_result_t value) {
        m_latestValue = value;
    });
    
    m_emitTimer = new QTimer(this);
}

void ReadableObservable::setInterval(std::chrono::milliseconds interval) {
    m_emitTimer->start(interval);
    
    emit intervalChanged(interval);
    
    connect(m_emitTimer, &QTimer::timeout, [=]() {
        emit valueUpdated(m_latestValue);
    });
}
