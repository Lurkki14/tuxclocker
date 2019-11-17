#include "ReadableObservable.h"

ReadableObservable::ReadableObservable(ReadableData *data, QObject *parent) : QObject(parent) {
    m_readableData = data;
    
    m_emitTimer = new QTimer;
}

void ReadableObservable::setInterval(std::chrono::milliseconds msecs) {
    m_emitTimer->start(msecs);
    
    connect(m_emitTimer, &QTimer::timeout, [=]() {
        emit valueUpdated(m_latestValue);
    });
}
