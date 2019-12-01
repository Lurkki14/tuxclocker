#pragma once

#include <QStyledItemDelegate>
#include <QStandardItem>

class AssignableEditorDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    AssignableEditorDelegate(QObject *parent = nullptr);
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget * editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    QSize m_customExpansionSize;
    bool m_customExpansionSizeNeeded;
};
