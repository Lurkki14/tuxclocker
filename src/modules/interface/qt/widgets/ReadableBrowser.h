#pragma once

#include <ReadableManager.h>

#include <QWidget>
#include <QDrag>
#include <QDropEvent>
#include <QVBoxLayout>
#include <QTreeView>

#include <ReadableItemModel.h>
#include <ReadableTreeView.h>

class ReadableBrowser : public QWidget {
    Q_OBJECT
public:
    ReadableBrowser(ReadableManager *readableManager, QWidget *parent = nullptr);
    ~ReadableBrowser();
signals:
    void itemDragStarted();
private:
    ReadableManager *m_readableManager; // ReadableManager instance that the browser is generated from
    
    QVBoxLayout *m_mainLayout;
    ReadableTreeView *m_readableTreeView;
    ReadableItemModel *m_browserModel;
    
    void genBrowserTree(ReadableTreeView *treeView, QStandardItemModel *itemModel);
    QStandardItem *addBrowserItem(tc_readable_node_t* node, QStandardItem *parent); // Return a newly created item that is a child of 'parent'
};
