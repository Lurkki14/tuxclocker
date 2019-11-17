#include "ReadableMasterObservable.h"

ReadableMasterObservable::ReadableMasterObservable(ReadableData *data, QObject *parent) : QObject(parent) {
    m_lowestInterval = std::chrono::milliseconds(-1);
    
    m_readableData = data;
    
    m_emitTimer = new QTimer;
}

void ReadableMasterObservable::notifyChangedInterval(std::chrono::milliseconds interval) {
    // Interval hasn't been set yet or new one is in the range 0 < new < old
    if ((m_lowestInterval < std::chrono::milliseconds(0) || interval < m_lowestInterval) && interval > std::chrono::milliseconds(0)) {
        m_lowestInterval = interval;
    }
    
    m_emitTimer->start(m_lowestInterval);
    
    connect(m_emitTimer, &QTimer::timeout, [=]() {
        emit valueUpdated(m_readableData->value());
    });
}

/*ReadableObservable *ReadableMasterObservable::observable(ReadableData *data, std::chrono::milliseconds updateInterval) {
    QTimer *timer = new QTimer;
    
    ReadableObservable *masterObservable = new ReadableObservable(data, this); // Create master observable for this node
    
    connect(timer, &QTimer::timeout, [=]() {
        
    });
}*/
