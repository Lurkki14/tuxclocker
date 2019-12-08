#pragma once

#include <QStyledItemDelegate>
#include <QStandardItem>
#include <QTreeView>

class AssignableEditorDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    AssignableEditorDelegate(QTreeView *treeView, QObject *parent = nullptr);
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    
    void setSpanAllColumns(bool on) const;
public slots:
    void setExpansionSize(QSize size) const;
    void setExpansionDisabled(bool off) const;
private:
    QSize m_customExpansionSize;
    bool m_customExpansionSizeNeeded;
    mutable bool m_spanAllColumns;
    QTreeView *m_treeView; // Tree view that the delegate belongs to
    
    // Store these for setting column span/row height on demand
    mutable QModelIndex m_editorIndex;
    mutable QWidget *m_editorWidget;
    mutable QStyleOptionViewItem m_viewItem;
    
    mutable QSize m_originalItemSize;
};
