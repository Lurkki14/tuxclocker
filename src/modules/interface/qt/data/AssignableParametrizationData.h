#pragma once

#include "AssignableData.h"
#include "ReadableData.h"

#include <QMetaType>
#include <QVector>

// Class for assignable editor to store data about parametrized assignables

class AssignableParametrizationData {
private:
    bool m_active; // Should target be currently parametrized
    ReadableData m_parameterReadable; // Readable node that the recepient is parametrized from
    AssignableData m_controlledAssignable;
    std::chrono::milliseconds m_updateInterval;
    QVector <qreal> m_xVector;
    QVector <qreal> m_yVector;
public:
    AssignableParametrizationData() {};
    AssignableParametrizationData(AssignableData &data) {m_controlledAssignable = data;}
};

Q_DECLARE_METATYPE(AssignableParametrizationData);
