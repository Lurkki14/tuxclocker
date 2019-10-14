#include "AssignableWidget.h"
#include <AssignableEditorDelegate.h>

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
    
    m_assignableEditor = new AssignableEditor;
    m_splitter->addWidget(m_assignableEditor);
    
    setLayout(m_mainLayout);
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
    
    tc_assignable_node_t *root= (tc_assignable_node_t*) nv_mod->category_data_callback();
    
    if (root == NULL) {
        return;
    }
    
    QStandardItemModel *assignableModel = new QStandardItemModel;
    
    
    std::function<void(tc_assignable_node_t*, QStandardItem*)> traverse;
    traverse = [=, &traverse](tc_assignable_node_t *node, QStandardItem *item) {
        if (node == NULL) {
            return;
        }
        
        if (node->name != NULL) { 
            qDebug() << node->name;
        }
        
        // List for adding the name and editor on the same row
        QList <QStandardItem*> rowItems;
        
        QStandardItem *nameItem = new QStandardItem;
        nameItem->setText(node->name);
        //item->appendRow(nameItem);
        rowItems.append(nameItem);
        
        QStandardItem *editorItem = new QStandardItem;
        editorItem->setText(node->name);
        AssignableData data(node);
        QVariant v;
        v.setValue(data);
        editorItem->setData(v);
        
        rowItems.append(editorItem);
        item->appendRow(rowItems);
        
        if (node->children_count == 0) {
            return;
        }
        
        for (uint32_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i], nameItem);
        }
    };
    
    connect(m_assignableTreeView, &QTreeView::activated, [=](QModelIndex index) {m_assignableEditor->setData(assignableModel->itemFromIndex(index)->data());});
    
    QStandardItem *parentItem = assignableModel->invisibleRootItem();
    
    // We don't want to display root nodes from the modules
    for (uint32_t i = 0; i < root->children_count; i++) {
        traverse(root->children_nodes[i], parentItem);
    }
    
    m_assignableTreeView->setModel(assignableModel);
    
    AssignableEditorDelegate *delegate = new AssignableEditorDelegate;
    
    //m_assignableTreeView->setItemDelegateForColumn(1, delegate);
    m_assignableTreeView->setItemDelegate(delegate);
    
    //m_assignableTreeView->setHeaderHidden(true);
    
    m_assignableTreeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
}
