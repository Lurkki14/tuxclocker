#pragma once

#include <AssignableData.h>

#include <QWidget>
#include <QSplitter>
#include <QLayout>
#include <QStandardItemModel>
#include <QTreeWidget>
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
    QTreeWidget *m_assignableTreeWidget;
    QTreeView *m_assignableTreeView;
    
    void genAssignableTree(QTreeView *treeView);
};
