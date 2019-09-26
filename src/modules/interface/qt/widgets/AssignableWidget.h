#pragma once

#include "AssignableEditor.h"

#include <QWidget>
#include <QSplitter>
#include <QLayout>
#include <QTreeView>
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
    AssignableEditor *m_assignableEditor;
    
    void genAssignableTree(QTreeView *treeView);
};
