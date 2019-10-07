#pragma once

#include <AssignableData.h>
#include "IntRangeEditor.h"
#include "EnumEditor.h"

#include <QWidget>
#include <QLayout>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>

class AssignableEditor : public QWidget {
    Q_OBJECT
public:
    AssignableEditor(QWidget *parent = nullptr);
    ~AssignableEditor();
    // Set the data for the editor
    void setData(const QVariant &data);
private:
    AssignableData m_assignableData;
    
    QGridLayout *m_mainLayout;
    QLabel *m_title;
    // Contains the node specific widgets
    QStackedWidget *m_editorStackedWidget;
    
    // Different widgets for editing different types of nodes
    // Empty widget
    QWidget *m_noneEditor;
    IntRangeEditor *m_intRangeEditor;
    EnumEditor *m_enumEditor;
};
