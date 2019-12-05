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
    void setEditorData(QWidget * editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    
    void setSpannedIndex(const QModelIndex &index) {m_spannedIndex = index;}
private:
    QModelIndex m_spannedIndex; // Index of the editor that should span all columns
    QSize m_customExpansionSize;
    bool m_customExpansionSizeNeeded;
    QTreeView *m_treeView; // Tree view that the delegate belongs to
};
