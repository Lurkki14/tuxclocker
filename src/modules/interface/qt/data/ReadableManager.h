#pragma once

#include <QVector>

#include <tc_module.h>
#include <tc_readable.h>

class ReadableManger {
public:
    ReadableManger();
    ~ReadableManger();
    QVector <tc_readable_node_t*> rootNodes() {return m_rootNodes;}
private:
    QVector <tc_readable_node_t*> m_rootNodes;
};
