#include "AssignableWidget.h"
#include <tc_module.h>
#include <tc_assignable.h>
#include <tc_common.h>
#include <QDebug>

AssignableWidget::AssignableWidget(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QGridLayout;
    
    m_splitter = new QSplitter;
    m_mainLayout->addWidget(m_splitter);
    
    m_assignableTreeWidget = new QTreeWidget;
    genAssignableTree(m_assignableTreeWidget);
    m_splitter->addWidget(m_assignableTreeWidget);
    
    m_assignableEditor = new AssignableEditor;
    m_splitter->addWidget(m_assignableEditor);
    
    setLayout(m_mainLayout);
}

AssignableWidget::~AssignableWidget() {
}

void AssignableWidget::genAssignableTree(QTreeWidget* treeWidget) {
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
    
    std::function<void(tc_assignable_node_t*, QTreeWidgetItem*)> traverse;
    traverse = [=, &traverse](tc_assignable_node_t *node, QTreeWidgetItem *item) {
        if (node == NULL) {
            return;
        }
        
        QTreeWidgetItem *newItem = new QTreeWidgetItem;
        if (node->name != NULL) { 
            qDebug() << node->name;
            
           newItem->setText(0, QString(node->name));
        }
        item->addChild(newItem);
        
        if (node->children_count == 0) {
            return;
        }
        
        for (uint32_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i], newItem);
        }
    };
    
    qDebug() << "traversing nodes";
    
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, root->name);
    m_assignableTreeWidget->addTopLevelItem(item);
    
    traverse(root, item);
    
    connect(m_assignableTreeWidget, &QTreeWidget::currentItemChanged, []() {qDebug("item changed");});
}
