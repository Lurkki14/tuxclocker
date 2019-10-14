#include "AssignableEditorDelegate.h"
#include "AssignableData.h"
#include <EnumEditor.h>
#include <IntRangeEditor.h>

#include <QDebug>

AssignableEditorDelegate::AssignableEditorDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

QWidget *AssignableEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Check which type the editor should be
    AssignableData data = qvariant_cast<AssignableData>(index.data());
    
    qDebug() << data.m_rangeInfo.int_range.max;
    
    switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_RANGE:
            return new IntRangeEditor(parent);
        case TC_ASSIGNABLE_ENUM:
            return new EnumEditor(parent);
        default:
            return nullptr;
    }
}

void AssignableEditorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    editor->setGeometry(option.rect);
}

void AssignableEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    
}
