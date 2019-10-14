#pragma once

#include <QMetaType>
#include <QString>
#include <tc_assignable.h>

// Contains the information for editing data for each node whose value can be modified. 

class AssignableData {
public:
    AssignableData();
    // Create the data from an assignable node
    AssignableData(const tc_assignable_node_t* node);
    
    ~AssignableData();
    
    // Returns the currently set value
   // QVariant currentValue();
    
//private:
    QString name;
    QString unit;
    const int8_t (*m_assignCallback)();
    
    tc_assignable_value_category m_valueCategory;
    union {
        tc_assignable_enum_t m_enumInfo;
        tc_assignable_range_t m_rangeInfo;
  };
};

// Declare as metatype for this to be used as a QVariant in QStandardItem
Q_DECLARE_METATYPE(AssignableData)
