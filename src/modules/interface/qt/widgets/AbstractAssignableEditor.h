#pragma once

// Defines the common interface for assignable editors

#include <QVariant>
#include <QWidget>

#include <AssignableData.h>

class AbstractAssignableEditor : public QWidget {
public:
    AbstractAssignableEditor(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual void setValue(QVariant value) = 0;
    virtual void setAssignableData(const AssignableData &data) = 0;
    virtual QVariant value() = 0;
    // Get the textual representation of the current value
    virtual QString text() = 0;
};
