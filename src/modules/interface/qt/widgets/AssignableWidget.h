#pragma once

#include "AssignableEditor.h"

#include <QWidget>
#include <QSplitter>
#include <QLayout>
#include <QTreeWidget>
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
    AssignableEditor *m_assignableEditor;
    
    void genAssignableTree(QTreeWidget *treeWidget);
};
