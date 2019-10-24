#include "AssignableEditorDelegate.h"
#include "AssignableData.h"
#include <EnumEditor.h>
#include <IntRangeEditor.h>

#include <QDebug>
#include <QPainter>

AssignableEditorDelegate::AssignableEditorDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

void AssignableEditorDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    /*if (index.data().canConvert<AssignableData>()) {
        // Display current value of item when not editing
        qDebug() << option.state;
        if (!(option.state & QStyle::State_Selected)) {
            AssignableData data = qvariant_cast<AssignableData>(index.data());
            painter->drawText(option.rect, Qt::AlignCenter, QString::number(data.value().toInt()));
        }
        
        //AssignableData data = qvariant_cast<AssignableData>(index.data());
        //painter->drawText(option.rect, Qt::AlignCenter, QString::number(data.value().toInt()));
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
    */
    
    QStyledItemDelegate::paint(painter, option, index);
}

QWidget *AssignableEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Check which type the editor should be
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index, Qt::UserRole));
    
    switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_RANGE:
            switch (data.m_rangeInfo.range_data_type) {
                case TC_ASSIGNABLE_RANGE_INT:
                    return new IntRangeEditor(parent, data);
                default:
                    return QStyledItemDelegate::createEditor(parent, option, index);
            }
        case TC_ASSIGNABLE_ENUM:
            return new EnumEditor(parent, data);
        default:
            return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void AssignableEditorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    editor->setGeometry(option.rect);
}

void AssignableEditorDelegate::setEditorData(QWidget* editor, const QModelIndex &index) const {
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index, Qt::UserRole));
    
    switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_RANGE:
            switch (data.m_rangeInfo.range_data_type) {
                case TC_ASSIGNABLE_RANGE_INT: {
                    IntRangeEditor *ed = static_cast<IntRangeEditor*>(editor);
                    ed->setValue(data.value().toInt());
                    break;
                }
                default:
                    break;
            }
        default:
            break;
    }
}

void AssignableEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index, Qt::UserRole));
    QVariant v;
    QStandardItemModel *s_model = static_cast<QStandardItemModel*>(model);
    
    switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_ENUM: {
            EnumEditor *ed = static_cast<EnumEditor*>(editor);
            data.setValue(ed->value());
            v.setValue(data);
            
            s_model->setData(index, v, Qt::UserRole);
            s_model->setData(index, ed->value(), Qt::DisplayRole);
            break;
            }
        case TC_ASSIGNABLE_RANGE:
            switch (data.m_rangeInfo.range_data_type) {
                case TC_ASSIGNABLE_RANGE_INT: {
                    IntRangeEditor *ed = static_cast<IntRangeEditor*>(editor);
                    data.setValue(ed->value());
                    v.setValue(data);
                    
                    s_model->setData(index, v, Qt::UserRole);
                    s_model->setData(index, ed->value(), Qt::DisplayRole);
                    break;
                    }
                default:
                    break;
            }
        default:
            break;
    }
}
