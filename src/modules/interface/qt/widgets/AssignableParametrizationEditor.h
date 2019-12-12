#pragma once

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QStackedWidget>

#include <AssignableParametrizationData.h>
#include "AbstractExpandableItemEditor.h"
#include "GraphEditor.h"

class AssignableParametrizationEditor : public AbstractExpandableItemEditor {
public:
    AssignableParametrizationEditor(QWidget *parent = nullptr);
    void setData(AssignableParametrizationData &data);
    AssignableParametrizationData data() {return m_parametrizationData;}
private:
    QVBoxLayout *m_layout;
    QCheckBox *m_enabledCheckBox;
    QPushButton *m_editButton;
    
    QStackedWidget *m_stackedWidget; // Displays editor and initial widget at different times
    GraphEditor *m_editorWidget; // Editor graph and Save/Cancel buttons
    QWidget *m_initialWidget; // Checkbox for enablement and button to start editing
    
    AssignableParametrizationData m_parametrizationData;
};
