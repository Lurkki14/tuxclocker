#pragma once

#include <QObject>
#include <QHash>
#include <QTimer>

#include "ReadableManager.h"
#include "ReadableData.h"
#include "ReadableObservable.h"

// Class that updates readable values from nodes and emits signals for observers. This prevents CPU overhead when the same value is being observed in multiple places.

class ReadableObservable;

class ReadableMasterObservable : public QObject {
public:
    ReadableMasterObservable(ReadableData *data, QObject *parent = nullptr);
    ~ReadableMasterObservable();
    void notifyChangedInterval(std::chrono::milliseconds interval); // Called when an observable changes its emission interval
    //ReadableObservable *observable(ReadableData *data, std::chrono::milliseconds updateInterval);
    ReadableObservable *createObservable(std::chrono::milliseconds updateInterval);
signals:
    void valueUpdated(tc_readable_result_t value);
private:
    Q_OBJECT
    
    ReadableData m_readableData; // Node to update value from
    std::chrono::milliseconds m_lowestInterval; // The lowest interval that an observer wants to update its value
    QTimer *m_emitTimer; // Updates the value
};
