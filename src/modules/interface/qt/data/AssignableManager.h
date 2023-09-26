#pragma once

#include <tc_assignable.h>
#include <tc_module.h>

#include <QVector>
#include <QList>
// Initializes, holds and closes the list of assignable modules 

class AssignableManager {
public:
    AssignableManager();
    ~AssignableManager();
    
    tc_assignable_node_t *first() {return m_assignableRootNodes[0];}
    
    // Return a list of root assignable nodes
    QList <tc_assignable_node_t*> rootNodes() {return m_assignableRootNodes;}
private:
    QVector <tc_module_t*> m_assignableModules;
    QList <tc_assignable_node_t*> m_assignableRootNodes;
    
    tc_assignable_node_t **m_rootNodes;
};
