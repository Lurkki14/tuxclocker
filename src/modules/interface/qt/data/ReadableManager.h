#pragma once

#include <QVector>

#include <tc_module.h>
#include <tc_readable.h>

class ReadableManager {
public:
    ReadableManager();
    ~ReadableManager();
    QVector <tc_readable_node_t*> rootNodes() {return m_rootNodes;}
    tc_readable_node_t *root() {return m_root;}
private:
    QVector <tc_readable_node_t*> m_rootNodes;
    tc_readable_node_t *m_root;
};
