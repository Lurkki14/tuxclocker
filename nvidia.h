#ifndef NVIDIA_H
#define NVIDIA_H

#include <QObject>

class nvidia : public QObject
{
    Q_OBJECT
public:
    explicit nvidia(QObject *parent = nullptr);

signals:

public slots:
};

#endif // NVIDIA_H