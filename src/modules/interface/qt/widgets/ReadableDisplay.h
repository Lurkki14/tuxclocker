#pragma once

#include <QWidget>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QVBoxLayout>

#include <ReadableData.h>
#include "ReadableGraphDisplay.h"

class ReadableDisplay : public QWidget {
public:
    ReadableDisplay(QWidget *parent = nullptr);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    QVBoxLayout *m_displayWidgetLayout;
};
