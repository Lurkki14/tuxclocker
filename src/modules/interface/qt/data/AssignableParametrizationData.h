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
    QVector <QPointF> m_pointsVector;
public:
    AssignableParametrizationData() {
        m_active = false;
    };
    AssignableParametrizationData(AssignableData &data) {
        m_controlledAssignable = data;
        m_active = false;
    }
    const AssignableData assignableData() {return m_controlledAssignable;}
    const QVector <QPointF> pointsVector() {return m_pointsVector;}
    void setPointsVector(const QVector <QPointF> vector) {m_pointsVector = vector;}
    bool enabled() {return m_active;}
    void setEnabled(bool enabled) {m_active = enabled;}
};

Q_DECLARE_METATYPE(AssignableParametrizationData);
