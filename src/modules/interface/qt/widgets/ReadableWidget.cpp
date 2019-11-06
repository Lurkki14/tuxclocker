#include "ReadableWidget.h"
#include <ReadableManager.h>

#include <QDebug>

ReadableWidget::ReadableWidget(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QVBoxLayout;
    
    m_tabWidget = new QTabWidget;
    
    m_readableBrowser = new ReadableBrowser;
    
    m_tabWidget->addTab(m_readableBrowser, "Browser");
    
    m_mainLayout->addWidget(m_tabWidget);
    
    setLayout(m_mainLayout);
    
    m_readableManager = new ReadableManger;
    qDebug() << "rd nodes" << m_readableManager->rootNodes();
}

ReadableWidget::~ReadableWidget() {
}
