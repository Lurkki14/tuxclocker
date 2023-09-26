#pragma once

#include <AssignableData.h>
#include <AssignableManager.h>
#include <AssignableParametrizationData.h>

#include <QWidget>
#include <QSplitter>
#include <QLayout>
#include <QStandardItemModel>
#include <QTreeView>
#include <QHeaderView>
#include <QTreeWidgetItem>

class AssignableWidget : public QWidget {
    Q_OBJECT
public:
    AssignableWidget(QWidget *parent = nullptr);
    ~AssignableWidget();

private:
    QGridLayout *m_mainLayout;
    // Splitter for editor and viewer
    QSplitter *m_splitter;
    QTreeView *m_assignableTreeView;
    
    // Assignable manager instance - maybe move this somewhere else?
    AssignableManager *m_assignableManager;
    
    void genAssignableTree(QTreeView *treeView);
    
    QStandardItem *addAssignableItem(tc_assignable_node_t *node, QStandardItem *parent);
};
