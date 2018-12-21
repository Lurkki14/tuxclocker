#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>
#include <QAbstractButton>
#include <QSettings>
#include <QListWidget>

namespace Ui {
class newProfile;
}

class newProfile : public QDialog
{
    Q_OBJECT

public:
    explicit newProfile(QWidget *parent = nullptr);
    ~newProfile();

signals:
    void mousePressEvent(QMouseEvent* event);

private slots:
    void on_saveButton_clicked();
    void on_profileNameEdit_textChanged(const QString &arg1);
    void on_cancelButton_clicked();
    void listProfiles();
    void editEntryName(QListWidgetItem *item);

    void on_addButton_pressed();

    void rightClick(QMouseEvent *event);
private:
    Ui::newProfile *ui;
    QString newProfileName;
};

#endif // NEWPROFILE_H
