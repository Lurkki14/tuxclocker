#include "AssignableWidget.h"
#include <AssignableEditorDelegate.h>
#include <AssignableManager.h>

#include <tc_module.h>
#include <tc_assignable.h>
#include <tc_common.h>
#include <QDebug>

AssignableWidget::AssignableWidget(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QGridLayout;
    
    m_splitter = new QSplitter;
    m_mainLayout->addWidget(m_splitter);
    
    m_assignableTreeView = new QTreeView;
    genAssignableTree(m_assignableTreeView);
    m_splitter->addWidget(m_assignableTreeView);
    
    setLayout(m_mainLayout);
    
    m_assignableManager = new AssignableManager;
}

AssignableWidget::~AssignableWidget() {
}

void AssignableWidget::genAssignableTree(QTreeView *treeView) {
    
    tc_module_t *nv_mod = tc_module_find(TC_CATEGORY_ASSIGNABLE, "nvidia");
    
    if (nv_mod != NULL) {
        if (nv_mod->init_callback() != TC_SUCCESS) {
            return;
        }
        printf("opened nv mod\n");
    }
    else {
        return;
    }
    
    tc_assignable_node_t *root = (tc_assignable_node_t*) nv_mod->category_data_callback();
    
    if (root == NULL) {
        return;
    }
    
    QStandardItemModel *assignableModel = new QStandardItemModel(0, 2);
    // Add header items
    QStandardItem *propertyHeader = new QStandardItem;
    propertyHeader->setText("Property");
    assignableModel->setHorizontalHeaderItem(0, propertyHeader);
    
    QStandardItem *valueHeader = new QStandardItem;
    valueHeader->setText("Value");
    assignableModel->setHorizontalHeaderItem(1, valueHeader);
    
    std::function<void(tc_assignable_node_t*, QStandardItem*)> traverse;
    traverse = [=, &traverse](tc_assignable_node_t *node, QStandardItem *item) {
        if (node == NULL) {
            return;
        }
        
        if (node->name != NULL) { 
            qDebug() << node->name;
        }
        
        QStandardItem *newItem = addAssignableItem(node, item);
        
        for (uint32_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i], newItem);
        }
    };
    
    QStandardItem *parentItem = assignableModel->invisibleRootItem();
    
    // Get root nodes from manager
    //QList <tc_assignable_node_t*> rootNodes = m_assignableManager->rootNodes();
    
    /*for (tc_assignable_node_t *root : rootNodes) {
        traverse(root, parentItem);
    }*/
        
    
    // We don't want to display root nodes from the modules
    for (uint32_t i = 0; i < root->children_count; i++) {
        traverse(root->children_nodes[i], parentItem);
    }
    
    m_assignableTreeView->setModel(assignableModel);
    
    AssignableEditorDelegate *delegate = new AssignableEditorDelegate;
    
    m_assignableTreeView->setItemDelegateForColumn(1, delegate);
    
    m_assignableTreeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
}

QStandardItem *AssignableWidget::addAssignableItem(tc_assignable_node_t *node, QStandardItem *parent) {
    if (!node || !node->name) {
        return nullptr;
    }
    
    QList <QStandardItem*> rowItems;
    
    QStandardItem *nameItem =  new QStandardItem;
    nameItem->setText(node->name);
    nameItem->setEditable(false);
    rowItems.append(nameItem);
    
    // Don't add editor item for TC_ASSIGNABLE_NONE nodes
    if (node->value_category != TC_ASSIGNABLE_NONE) {
        QStandardItem *editorItem = new QStandardItem;
        QVariant v;
        v.setValue(AssignableData(node));
        editorItem->setData(v, Qt::UserRole);
        //editorItem->setText(node->name);
        rowItems.append(editorItem);
    }
    parent->appendRow(rowItems);
    
    return nameItem;
}
