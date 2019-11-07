#pragma once

#include <ReadableManager.h>

#include <QWidget>
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>

class ReadableBrowser : public QWidget {
public:
    ReadableBrowser(ReadableManager *readableManager, QWidget *parent = nullptr);
    ~ReadableBrowser();
private:
    ReadableManager *m_readableManager; // ReadableManager instance that the browser is generated from
    
    QVBoxLayout *m_mainLayout;
    QTreeView *m_readableTreeView;
    QStandardItemModel *m_browserModel;
    
    void genBrowserTree(QTreeView *treeView, QStandardItemModel *itemModel);
    void addBrowserItem(tc_readable_node_t* node, QStandardItem *parent); // Create a new item from 'node'
};
