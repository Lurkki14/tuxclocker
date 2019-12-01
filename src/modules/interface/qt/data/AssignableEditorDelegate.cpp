#include "AssignableEditorDelegate.h"
#include "AssignableData.h"
#include "AssignableParametrizationData.h"
#include <AbstractAssignableEditor.h>
#include <AssignableParametrizationEditor.h>
#include <EnumEditor.h>
#include <IntRangeEditor.h>

#include <QDebug>

AssignableEditorDelegate::AssignableEditorDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

QWidget *AssignableEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant v_data = index.model()->data(index, Qt::UserRole);
    qDebug() << v_data;
    // Widget for parametrization data
    if (v_data.canConvert<AssignableParametrizationData>()) {
        return new AssignableParametrizationEditor(parent);
    }
    
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
    //editor->setGeometry(QRect(QPoint(option.rect.topLeft()), QPoint(500, 500)));
    //editor->setGeometry(option.widget->rect());
}

void AssignableEditorDelegate::setEditorData(QWidget* editor, const QModelIndex &index) const {
    QVariant v_data = index.model()->data(index, Qt::UserRole);
    
    if (v_data.canConvert<AssignableData>()) {
        AssignableData data = qvariant_cast<AssignableData>(v_data);
        AbstractAssignableEditor *a_editor = static_cast<AbstractAssignableEditor*>(editor);
        a_editor->setAssignableData(data);
        a_editor->setValue(data.value());
    }
}

void AssignableEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QVariant v_data = model->data(index, Qt::UserRole);
    
    if (v_data.canConvert<AssignableData>()) {
        AbstractAssignableEditor *a_editor = static_cast<AbstractAssignableEditor*>(editor);
        AssignableData data = qvariant_cast<AssignableData>(v_data);
        data.setValue(a_editor->value());
        
        QVariant v;
        v.setValue(data);
        
        model->setData(index, v, Qt::UserRole);
        model->setData(index, a_editor->text(), Qt::DisplayRole);
    }
}

QSize AssignableEditorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    //qDebug() << option.widget;
    
    return QStyledItemDelegate::sizeHint(option, index);
    
    //return QSize(150, 150);
}
