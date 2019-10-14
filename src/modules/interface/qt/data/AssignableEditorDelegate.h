#pragma once

#include <QStyledItemDelegate>

class AssignableEditorDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    AssignableEditorDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};
