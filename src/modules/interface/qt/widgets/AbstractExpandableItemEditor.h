#pragma once

// Interface for editor widgets that cannot fit into the space normally provided by a delegate or otherwise need expanding.

#include <QWidget>

class AbstractExpandableItemEditor : public QWidget {
public:
    AbstractExpandableItemEditor(QWidget *parent) : QWidget(parent) {}
signals:
    void expansionSizeRequested(const QSize size);
    void expansionDisableRequested();
private:
    Q_OBJECT
};
