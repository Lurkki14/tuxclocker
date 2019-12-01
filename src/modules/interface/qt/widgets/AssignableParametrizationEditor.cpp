#include "AssignableParametrizationEditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

AssignableParametrizationEditor::AssignableParametrizationEditor(QWidget *parent) : QWidget(parent) {
    setLayout(new QVBoxLayout);
    
    layout()->addWidget(new QLabel("Enable"));
    layout()->addWidget((m_enabledCheckBox = new QCheckBox));
    layout()->addWidget((m_editButton = new QPushButton("Edit")));
    
    layout()->setMargin(0);
    
    setAutoFillBackground(true);
}
