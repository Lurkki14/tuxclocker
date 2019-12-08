#include "AbstractExpandableItemEditor.h"
#include "AssignableEditorDelegate.h"
#include "AssignableData.h"
#include "AssignableParametrizationData.h"
#include <AbstractAssignableEditor.h>
#include <AssignableParametrizationEditor.h>
#include <EnumEditor.h>
#include <IntRangeEditor.h>

#include <QDebug>
#include <QTreeView>

AssignableEditorDelegate::AssignableEditorDelegate(QTreeView *treeView, QObject *parent) : QStyledItemDelegate(parent) {
    m_treeView = treeView;
    m_spanAllColumns = false;
}

QWidget *AssignableEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    m_editorIndex = index;
    m_viewItem = option;
    
    m_originalItemSize = index.data(Qt::SizeHintRole).toSize();
    
    QVariant v_data = index.model()->data(index, Qt::UserRole);
    // Widget for parametrization data
    if (v_data.canConvert<AssignableParametrizationData>()) {
        auto a_editor = new AssignableParametrizationEditor(parent);
        connect(a_editor, &AbstractExpandableItemEditor::expansionSizeRequested, [=](const QSize size) {
            qDebug() << "expansion requested";
            setExpansionSize(size);
            setSpanAllColumns(true);
        });
        
        connect(a_editor, &AbstractExpandableItemEditor::expansionDisableRequested, [=]() {
            setSpanAllColumns(false);
            setExpansionDisabled(true);
        });
        
        return a_editor;
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
    if (m_spanAllColumns) {
        // Span editor across all columns        
        const int maxCol = index.model()->columnCount(index.parent());
        QRect tempRect = m_treeView->visualRect(index.model()->index(index.row(), 0, index.parent()));
        int top = tempRect.top();
        int left = tempRect.left();
        int bottom = tempRect.bottom();
        int right= tempRect.right();
        
        for(int i = 1; i < maxCol ; i++){
            tempRect = m_treeView->visualRect(index.model()->index(index.row(), i, index.parent()));
            if (tempRect.top()<top) {
                top = tempRect.top();
            }
            if (Q_UNLIKELY(tempRect.left() < left)) {
                left= tempRect.left();
            }
            if (tempRect.bottom() > bottom) {
                bottom = tempRect.bottom();
            }
            if (Q_LIKELY(tempRect.right() > right)) {
                right= tempRect.right();
            }
        }
        editor->setGeometry(QRect(QPoint(left, top),QPoint(right, bottom)));
    }
    else {
        editor->setGeometry(option.rect);
        m_editorWidget = editor;
    }
}

void AssignableEditorDelegate::setEditorData(QWidget* editor, const QModelIndex &index) const {
    QVariant v_data = index.model()->data(index, Qt::UserRole);
    
    if (v_data.canConvert<AssignableData>()) {
        AssignableData data = qvariant_cast<AssignableData>(v_data);
        AbstractAssignableEditor *a_editor = static_cast<AbstractAssignableEditor*>(editor);
        a_editor->setAssignableData(data);
        a_editor->setValue(data.value());
        return;
    }
    
    if (v_data.canConvert<AssignableParametrizationData>()) {
        auto data = qvariant_cast<AssignableParametrizationData>(v_data);
        static_cast<AssignableParametrizationEditor*>(editor)->setData(data);
        return;
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
    
    // Set original size
    model->setData(index, m_originalItemSize, Qt::SizeHintRole);
    m_spanAllColumns = false;
}

QSize AssignableEditorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    return QStyledItemDelegate::sizeHint(option, index);
}

void AssignableEditorDelegate::setExpansionSize(QSize size) const {
    qDebug() << "set size hint to" << size;
    auto model = const_cast<QAbstractItemModel*>(m_editorIndex.model());
    if (!model) {
        return;
    }
    model->setData(m_editorIndex, size, Qt::SizeHintRole);
}

void AssignableEditorDelegate::setSpanAllColumns(bool on) const {
    m_spanAllColumns = on;
    qDebug() << on;
    updateEditorGeometry(m_editorWidget, m_viewItem, m_editorIndex);
}

void AssignableEditorDelegate::setExpansionDisabled(bool off) const {
    auto model = const_cast<QAbstractItemModel*>(m_editorIndex.model());
    if (!model) {
        return;
    }
    if (off) {
        // FIXME : this doesn't work properly if the column is resized during expansion
        model->setData(m_editorIndex, m_originalItemSize, Qt::SizeHintRole);
    }
}
