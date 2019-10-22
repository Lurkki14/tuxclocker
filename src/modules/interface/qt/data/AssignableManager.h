#include <tc_assignable.h>
#include <tc_module.h>

#include <QVector>
// Initializes, holds and closes the list of assignable modules 

class AssignableManager {
public:
    AssignableManager();
    ~AssignableManager();
    
    // Return a list of root assignable nodes
    QList <tc_assignable_node_t*> rootNodes();
private:
    QVector <tc_module_t*> m_assignableModules;
    QVector <tc_assignable_node_t*> m_assignableRootNodes;
};
