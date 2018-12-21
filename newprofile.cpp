#include "newprofile.h"
#include "ui_newprofile.h"

newProfile::newProfile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::newProfile)
{
    ui->setupUi(this);
}

newProfile::~newProfile()
{
    delete ui;
}
