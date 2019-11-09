#pragma once

#include <ReadableManager.h>

#include <QWidget>
#include <QDrag>
#include <QDropEvent>
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>

class ReadableBrowser : public QWidget {
    Q_OBJECT
public:
    ReadableBrowser(ReadableManager *readableManager, QWidget *parent = nullptr);
    ~ReadableBrowser();
signals:
    void itemDragStarted(QString &text);
private:
    ReadableManager *m_readableManager; // ReadableManager instance that the browser is generated from
    
    QVBoxLayout *m_mainLayout;
    QTreeView *m_readableTreeView;
    QStandardItemModel *m_browserModel;
    
    void genBrowserTree(QTreeView *treeView, QStandardItemModel *itemModel);
    QStandardItem *addBrowserItem(tc_readable_node_t* node, QStandardItem *parent); // Return a newly created item that is a child of 'parent'
    
    void mousePressEvent(QMouseEvent *event); // Start the drag event for an item
};
