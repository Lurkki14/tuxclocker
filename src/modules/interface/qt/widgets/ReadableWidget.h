#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include "ReadableBrowser.h"
#include "ReadableDisplay.h"
#include <ReadableManager.h>

class ReadableWidget : public QWidget {
    Q_OBJECT
public:
    ReadableWidget(QWidget *parent = nullptr);
    ~ReadableWidget();
private:
    // Tab widget for browser and viewer
    QTabWidget *m_tabWidget;
    ReadableBrowser *m_readableBrowser;
    
    QVBoxLayout *m_mainLayout;
    
    ReadableManager *m_readableManager;
    ReadableDisplay *m_readableDisplay;
};
