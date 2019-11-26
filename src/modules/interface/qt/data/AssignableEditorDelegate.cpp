#include "AssignableEditorDelegate.h"
#include "AssignableData.h"
#include "AbstractAssignableEditor.h"
#include <EnumEditor.h>
#include <IntRangeEditor.h>

#include <QDebug>

AssignableEditorDelegate::AssignableEditorDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

QWidget *AssignableEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Check which type the editor should be
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index, Qt::UserRole));
 
    AbstractAssignableEditor *editor = nullptr;
    
    switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_RANGE:
            switch (data.m_rangeInfo.range_data_type) {
                case TC_ASSIGNABLE_RANGE_INT:
                    editor = new IntRangeEditor(parent);
                    return editor;
                default:
                    return nullptr;
            }
        case TC_ASSIGNABLE_ENUM:
            editor = new EnumEditor(parent);
            return editor;
        default:
            return nullptr;
    }
}

void AssignableEditorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    editor->setGeometry(option.rect);
}

void AssignableEditorDelegate::setEditorData(QWidget* editor, const QModelIndex &index) const {
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index, Qt::UserRole));
    
    AbstractAssignableEditor *a_editor = static_cast<AbstractAssignableEditor*>(editor);
    a_editor->setAssignableData(data);
    a_editor->setValue(data.value());
}

void AssignableEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index, Qt::UserRole));
    QVariant v;
    QStandardItemModel *s_model = static_cast<QStandardItemModel*>(model);
    
    AbstractAssignableEditor *a_editor = static_cast<AbstractAssignableEditor*>(editor);
    
    data.setValue(a_editor->value());
    v.setValue(data);
    
    s_model->setData(index, v, Qt::UserRole);
    s_model->setData(index, a_editor->text(), Qt::DisplayRole);
}
