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
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index));
    
    qDebug() << data.m_valueCategory;
    
    switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_RANGE:
            switch (data.m_rangeInfo.range_data_type) {
                case TC_ASSIGNABLE_RANGE_INT:
                    return new IntRangeEditor(parent, data);
                default:
                    return nullptr;
            }
        case TC_ASSIGNABLE_ENUM:
            return new EnumEditor(parent, data);
        default:
            return nullptr;
    }
}

void AssignableEditorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    editor->setGeometry(option.rect);
}

void AssignableEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    AssignableData data = qvariant_cast<AssignableData>(index.model()->data(index));
    QStandardItemModel *s_model = qobject_cast<QStandardItemModel*>(model);
    
    /*switch (data.m_valueCategory) {
        case TC_ASSIGNABLE_ENUM: {
            EnumEditor *ed = qobject_cast<EnumEditor*>(editor);
            // Set the editor item text to current selection
            s_model->itemFromIndex(index)->setText(ed->value());
            break;
            }
        case TC_ASSIGNABLE_RANGE:
            switch (data.m_rangeInfo.range_data_type) {
                case TC_ASSIGNABLE_RANGE_INT: {
                    IntRangeEditor *ed = qobject_cast<IntRangeEditor*>(editor);
                    s_model->itemFromIndex(index)->setText(QString(ed->value()));
                    break;
                    }
                default:
                    break;
            }
        default:
            break;
    }*/
}
