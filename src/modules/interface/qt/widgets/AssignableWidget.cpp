#include "AssignableWidget.h"
#include <tc_module.h>
#include <tc_assignable.h>
#include <tc_common.h>
#include <QDebug>

AssignableWidget::AssignableWidget(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QGridLayout;
    
    m_splitter = new QSplitter;
    m_mainLayout->addWidget(m_splitter);
    
    m_assignableTreeView = new QTreeView;
    //genAssignableTree(m_assignableTreeView);
    m_splitter->addWidget(m_assignableTreeView);
    
    m_assignableEditor = new AssignableEditor;
    m_splitter->addWidget(m_assignableEditor);
    
    setLayout(m_mainLayout);
}

AssignableWidget::~AssignableWidget() {
}

void AssignableWidget::genAssignableTree(QTreeView* treeView) {
    tc_module_t *nv_mod = tc_module_find(TC_CATEGORY_ASSIGNABLE, "nvidia");
    
    if (nv_mod != NULL) {
        if (nv_mod->init_callback() != TC_SUCCESS) {
            return;
        }
        printf("opened nv mod\n");
    }
    
    QTreeWidgetItem *item = new QTreeWidgetItem;
    tc_assignable_node_t *parent = (tc_assignable_node_t*) nv_mod->category_data_callback();
    
    if (parent == NULL) {
        return;
    }
    
    std::function<void(tc_assignable_node_t*)> traverse;
    traverse = [=, &traverse](tc_assignable_node_t *node) {
        if (node == NULL) {
            return;
        }
        
        if (node->name != NULL) { 
            qDebug() << node->name;
        }
        if (node->children_count == 0) {
            return;
        }
        for (uint32_t i = 0; i < node->children_count; i++) {
            traverse(node->children_nodes[i]);
        }
    };
    
    qDebug() << "traversing nodes" << parent->children_count << parent << parent->children_nodes[0];
    
    traverse(parent);
}
