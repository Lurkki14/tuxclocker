#include "ReadableBrowser.h"

#include <QMouseEvent>
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
        QStandardItem *newItem = addBrowserItem(node, item);
        
        if (!node->constant && node->value_callback) {
            tc_readable_result_t res = node->value_callback(node);
            qDebug() << res.valid << res.data.uint_value;
        }
        
        for (uint16_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i], newItem);
        }
    };
    //tc_readable_node_t *root = m_readableManager->root();
    QVector <tc_readable_node_t*> rootNodes  = m_readableManager->rootNodes();
    
    for (tc_readable_node_t *node : m_readableManager->rootNodes()) {
        // Start traversal from the first subitems of the root item
        for (uint16_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i], parentItem);
        }
    }
    
    treeView->setModel(itemModel);
    
    treeView->setDragEnabled(true);
}

QStandardItem *ReadableBrowser::addBrowserItem(tc_readable_node_t* node, QStandardItem *parent) {
    // Don't add item for constants
    if (node->constant) {
        return nullptr;
    }
    if (!node->name) {
        return nullptr;
    }
    QStandardItem *item = new QStandardItem;
    item->setText(node->name);
    
    item->setDragEnabled(true);
    
    parent->appendRow(item);
    
    return item;
}

void ReadableBrowser::mousePressEvent(QMouseEvent *event) {
    /*if (event->button() == Qt::LeftButton) {
        QDrag *drag = new QDrag(this);
        Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
        qDebug() << drag->mimeData();
    }*/
}
