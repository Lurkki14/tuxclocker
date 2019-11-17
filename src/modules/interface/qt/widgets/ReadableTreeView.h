#pragma once

#include <QTreeView>

// Enables the tree view to send a signal when item dragging starts

class ReadableTreeView : public QTreeView {
public:
    ReadableTreeView(QWidget *parent = nullptr) : QTreeView(parent) {}
signals:
    void itemDragStarted();
protected:
    void startDrag(Qt::DropActions acts) {
        emit itemDragStarted();
        
        QAbstractItemView::startDrag(acts);
    }
private:
    Q_OBJECT
};
