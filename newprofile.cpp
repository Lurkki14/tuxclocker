#include "newprofile.h"
#include "ui_newprofile.h"
#include "mainwindow.h"

newProfile::newProfile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::newProfile)
{
    ui->setupUi(this);
    listProfiles();

    connect(ui->profileList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(editEntryName(QListWidgetItem*)));

    SignalItemDelegate *delegate = new SignalItemDelegate(ui->profileList);
    deleg = delegate;
    connect(deleg, SIGNAL(editStarted), SLOT(testi()));
}

newProfile::~newProfile()
{
    delete ui;
}

void newProfile::on_profileNameEdit_textChanged(const QString &arg1)
{
    newProfileName = arg1;
}
void newProfile::testi()
{
    qDebug() << "edit started";
}
void newProfile::on_saveButton_clicked()
{
    QSettings settings("nvfancurve");
    // for loop here
    settings.beginGroup(newProfileName);
    // Add saving the GPU default values here
}

void newProfile::listProfiles()
{
    QSettings settings("nvfancurve");
    QString isProfile = "/isProfile";
    QStringList groups = settings.childGroups();
    qDebug() << "testi";
    for (int i=0; i<groups.length(); i++) {
        // Make a query $profile/isProfile
        QString group = groups[i];
        QString query = group.append(isProfile);
        qDebug() << query;
        if (settings.value(query).toBool()) {
            qDebug() << groups[i];
            ui->profileList->addItem(groups[i]);
        }
    }
    // Make the items editable
    for (int i=0; i<ui->profileList->count(); i++) {
        ui->profileList->item(i)->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
    }
}

void newProfile::editEntryName(QListWidgetItem *item)
{
    qDebug() << "item dblclicked";
    SignalItemDelegate *delegate = new SignalItemDelegate(ui->profileList);
    connect(delegate, &SignalItemDelegate::editStarted,[](){qDebug("edit started");});
    connect(delegate, &SignalItemDelegate::editFinished,[](){qDebug("edit finished");});
    ui->profileList->editItem(item);
}
void newProfile::rightClick(QMouseEvent *event)
{
    qDebug() << "press";
}

void newProfile::on_cancelButton_clicked()
{
    close();
}
void newProfile::on_addButton_pressed()
{
    /*SignalItemDelegate *delegate = new SignalItemDelegate(ui->profileList);
    connect(delegate, &SignalItemDelegate::editStarted,[](){qDebug("edit started");});
    connect(delegate, &SignalItemDelegate::editFinished,[](){qDebug("edit finished");});
    ui->profileList->setItemDelegate(delegate);
    ui->profileList->addItem("");
    int itemCount = ui->profileList->count()-1;
    ui->profileList->item(itemCount)->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
    ui->profileList->editItem(ui->profileList->item(itemCount)); */
}
