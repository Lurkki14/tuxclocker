#include "AssignableData.h"

AssignableData::AssignableData(const tc_assignable_node_t *node) {
    // Copy the values used by the editor
    m_valueCategory = node->value_category;
    switch (m_valueCategory) {
        case TC_ASSIGNABLE_ENUM:
            m_enumInfo = node->enum_info;
            break;
        case TC_ASSIGNABLE_RANGE:
            m_rangeInfo = node->range_info;
            break;
        default:
            break;
    }
    name = QString(node->name);
    unit = QString(node->unit);
}

AssignableData::AssignableData() {
}

AssignableData::~AssignableData() {
}
