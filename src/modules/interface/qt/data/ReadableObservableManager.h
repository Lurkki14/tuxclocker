#pragma once

// Holds a list of MasterObservables so they aren't duplicated and creates Observables

#include "ReadableMasterObservable.h"
#include <ReadableData.h>

#include <QHash>

class ReadableObservableManager {
public:
    ReadableObservableManager() {};
    // Create master observable for data if doesn't exist, and return an observable connected to it
    ReadableObservable *observable(ReadableData *data, std::chrono::milliseconds updateInterval);
private:
    QHash <const tc_readable_node_t*, ReadableMasterObservable*> m_nodeMap; // Hash for checking if there already is a MasterObservable with a node
};
