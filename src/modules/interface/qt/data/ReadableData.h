#pragma once

#include <QMetaType>
#include <QString>

#include <tc_readable.h>

class ReadableData {
private:
    tc_readable_node_t *m_readableNode;
public:
    ReadableData(tc_readable_node_t *node) {m_readableNode = node;}
    static QString mimeType() {return QString("tc-readable-data");}
    bool isConstant() {return m_readableNode->constant;}
    QString name() {return QString(m_readableNode->name);}
    QString unit() {return QString(m_readableNode->unit);}
    const tc_variant_t constData() {
        if (!isConstant()) {
            tc_variant_t retval;
            retval.data_type = TC_TYPE_NONE;
            return retval;
        }
        return m_readableNode->data;
    }
    
    const tc_readable_result_t value() {
        if (isConstant() || !m_readableNode->value_callback) {
            return tc_readable_result_create(TC_TYPE_NONE, NULL, 0);
        }
        return m_readableNode->value_callback(m_readableNode);
    }
};
