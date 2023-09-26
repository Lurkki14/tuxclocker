#include "ReadableMasterObservable.h"

#include <QDebug>

ReadableMasterObservable::ReadableMasterObservable(ReadableData *data, QObject *parent) : QObject(parent) {
    m_lowestInterval = std::chrono::milliseconds(-1);
    
    //m_readableData = data;
    
    m_readableData = ReadableData(*data);
    
    m_emitTimer = new QTimer;
}

ReadableMasterObservable::~ReadableMasterObservable() {
    delete m_emitTimer;
}

void ReadableMasterObservable::notifyChangedInterval(std::chrono::milliseconds interval) {
    // Interval hasn't been set yet or new one is in the range 0 < new < old
    if ((m_lowestInterval < std::chrono::milliseconds(0) || interval < m_lowestInterval) && interval > std::chrono::milliseconds(0)) {
        m_lowestInterval = interval;
    }
    
    m_emitTimer->start(m_lowestInterval);
    
    connect(m_emitTimer, &QTimer::timeout, [=]() {
        emit valueUpdated(m_readableData.value());
    });
}

ReadableObservable *ReadableMasterObservable::createObservable(std::chrono::milliseconds updateInterval) {
    ReadableObservable *observable = new ReadableObservable(this, this); // Set as parent so it gets deleted along with this object and we can keep track of it
    observable->setInterval(updateInterval);
    
    notifyChangedInterval(updateInterval);
    
    return observable;
}
