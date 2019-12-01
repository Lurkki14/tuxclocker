#include "ReadableObservableManager.h"

ReadableObservable *ReadableObservableManager::observable(ReadableData *data, std::chrono::milliseconds updateInterval) {
    if (m_nodeMap.contains(data->node())) {
        // MasterObservable for this node exists
        ReadableMasterObservable *mo = m_nodeMap.value(data->node());
        
        return mo->createObservable(updateInterval);
    }
    ReadableMasterObservable *mo = new ReadableMasterObservable(data);
    
    m_nodeMap.insert(data->node(), mo);
    
    return mo->createObservable(updateInterval);
}
