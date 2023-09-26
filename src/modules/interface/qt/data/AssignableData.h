#pragma once

#include <QMetaType>
#include <QString>
#include <QVariant>
#include <tc_assignable.h>

// Contains the information for editing data for each node whose value can be modified. 

class AssignableData {
public:
    AssignableData();
    // Create the data from an assignable node
    AssignableData(const tc_assignable_node_t* node);
    ~AssignableData();
    
    // Return type for getting the value category and assignable type
    struct AssignableTypeInfo {
        tc_assignable_value_category m_valueCategory;
        union {
            tc_assignable_enum_t m_enumInfo;
            tc_assignable_range_t m_rangeInfo;
        };
    };
    
    // Returns the currently set value
    QVariant value();
    void setValue(const QVariant &value);
//private:
    QString name;
    QString unit;
    const int8_t (*m_assignCallback)();
    
    tc_assignable_value_category m_valueCategory;
    union {
        tc_assignable_enum_t m_enumInfo;
        tc_assignable_range_t m_rangeInfo;
    };
private:
    // Stores the latest value
    QVariant m_currentValue;
};

// Declare as metatype for this to be used as a QVariant in QStandardItem
Q_DECLARE_METATYPE(AssignableData)

