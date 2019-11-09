#include "ReadableWidget.h"
#include <ReadableManager.h>

#include <QDebug>

ReadableWidget::ReadableWidget(QWidget *parent) : QWidget(parent) {
    m_mainLayout = new QVBoxLayout;
    
    m_tabWidget = new QTabWidget;
    
    m_readableManager = new ReadableManager;
    qDebug() << "rd nodes" << m_readableManager->rootNodes();
    
    m_readableBrowser = new ReadableBrowser(m_readableManager);
    
    m_readableDisplay = new ReadableDisplay;
    
    m_tabWidget->addTab(m_readableBrowser, "Browser");
    m_tabWidget->addTab(m_readableDisplay, "Display");
    
    m_mainLayout->addWidget(m_tabWidget);
    
    setLayout(m_mainLayout);
    
}

ReadableWidget::~ReadableWidget() {
}
