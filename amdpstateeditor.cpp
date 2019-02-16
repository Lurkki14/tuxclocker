#include "amdpstateeditor.h"
#include "ui_amdpstateeditor.h"

amdPstateEditor::amdPstateEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::amdPstateEditor)
{
    ui->setupUi(this);
    amdptr = new amd;
    types = amdptr;
    generateUI();

}

amdPstateEditor::~amdPstateEditor()
{
    delete ui;
}
void amdPstateEditor::generateUI()
{
    QWidget *lower = new QWidget;
    QWidget *upper = new QWidget;
    QHBoxLayout *ulo = new QHBoxLayout;
    QHBoxLayout *llo = new QHBoxLayout;
    for (int i=0; i<types->GPUList[0].corecloks.size(); i++) {
        QGridLayout *glo = new QGridLayout;
        QLabel *lb = new QLabel;
        QLabel *lb1 = new QLabel;
        QLabel *lb2 = new QLabel;
        lb->setText("mV");
        lb1->setText("MHz");
        lb2->setText("Core pstate "+QString::number(i));
        QSlider *sl = new QSlider;
        QSlider *sl1 = new QSlider;
        QSpinBox *sp = new QSpinBox;
        QSpinBox *sp1 = new QSpinBox;
        glo->addWidget(lb2);
        glo->addWidget(lb1, 1, 0);
        glo->addWidget(lb, 1, 1);
        glo->addWidget(sl, 2, 0);
        glo->addWidget(sl1, 2, 1);
        glo->addWidget(sp, 3, 0);
        glo->addWidget(sp1, 3, 1);
        QWidget *slowdg = new QWidget;
        slowdg->setLayout(glo);
        llo->addWidget(slowdg);
    }

    for (int i=0; i<types->GPUList[0].memclocks.size(); i++) {
        QGridLayout *glo = new QGridLayout;
        QLabel *lb = new QLabel;
        QLabel *lb1 = new QLabel;
        QLabel *lb2 = new QLabel;
        lb->setText("mV");
        lb1->setText("MHz");
        lb2->setText("Memory pstate "+QString::number(i));
        QSlider *sl = new QSlider;
        QSlider *sl1 = new QSlider;
        QSpinBox *sp = new QSpinBox;
        QSpinBox *sp1 = new QSpinBox;
        glo->addWidget(lb2);
        glo->addWidget(lb1, 1, 0);
        glo->addWidget(lb, 1, 1);
        glo->addWidget(sl, 2, 0);
        glo->addWidget(sl1, 2, 1);
        glo->addWidget(sp, 3, 0);
        glo->addWidget(sp1, 3, 1);
        QWidget *slowdg = new QWidget;
        slowdg->setLayout(glo);
        ulo->addWidget(slowdg);
    }
    lower->setLayout(ulo);
    upper->setLayout(llo);

    QVBoxLayout *mainlo = new QVBoxLayout(this);
    mainlo->addWidget(upper);
    mainlo->addWidget(lower);
    ui->centralWidget->setLayout(mainlo);
}
