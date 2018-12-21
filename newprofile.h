#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>

namespace Ui {
class newProfile;
}

class newProfile : public QDialog
{
    Q_OBJECT

public:
    explicit newProfile(QWidget *parent = nullptr);
    ~newProfile();

private:
    Ui::newProfile *ui;
};

#endif // NEWPROFILE_H
