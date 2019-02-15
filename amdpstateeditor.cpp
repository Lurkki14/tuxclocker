#include "amdpstateeditor.h"
#include "ui_amdpstateeditor.h"

amdPstateEditor::amdPstateEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::amdPstateEditor)
{
    ui->setupUi(this);
    generateUI();
}

amdPstateEditor::~amdPstateEditor()
{
    delete ui;
}
void amdPstateEditor::generateUI()
{
    QHBoxLayout *coreClkPstateLayout = new QHBoxLayout;
    for (int i=0; i<5; i++) {
        QSlider *sl = new QSlider;
        sl->setOrientation(Qt::Vertical);
        coreClkPstateLayout->addWidget(sl);
    }
    ui->coreClkPstateView->setLayout(coreClkPstateLayout);
    ui->memClkPstateView->setLayout(coreClkPstateLayout);
}
