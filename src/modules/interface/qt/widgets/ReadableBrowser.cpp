#include "ReadableBrowser.h"

#include <QDebug>

ReadableBrowser::ReadableBrowser(ReadableManager *readableManager, QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QVBoxLayout;
    
    m_readableTreeView = new QTreeView;
    
    m_browserModel = new QStandardItemModel;
    
    m_readableManager = readableManager;
    
    genBrowserTree(m_readableTreeView, m_browserModel);
    
    m_mainLayout->addWidget(m_readableTreeView);
    
    setLayout(m_mainLayout);
    
}

ReadableBrowser::~ReadableBrowser() {
}

void ReadableBrowser::genBrowserTree(QTreeView *treeView, QStandardItemModel *itemModel) {
    QStandardItem *parentItem = itemModel->invisibleRootItem();
    
    std::function<void(tc_readable_node_t*, QStandardItem*)> traverse;
    traverse = [=, &traverse](tc_readable_node_t *node, QStandardItem *item) {
        if (node->name) {
            qDebug() << node->name;
        }
        addBrowserItem(node, item);
        
        for (uint16_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i], item);
        }
    };
    //tc_readable_node_t *root = m_readableManager->root();
    QVector <tc_readable_node_t*> rootNodes  = m_readableManager->rootNodes();
    
    for (tc_readable_node_t *node : m_readableManager->rootNodes()) {
        traverse(node, parentItem);
    }
    
    treeView->setModel(itemModel);
}

void ReadableBrowser::addBrowserItem(tc_readable_node_t* node, QStandardItem *parent) {
    // Don't add item for constants
    if (node->constant) {
        return;
    }
    if (!node->name) {
        return;
    }
    QStandardItem *item = new QStandardItem;
    item->setText(node->name);
    
    parent->appendRow(item);
}
