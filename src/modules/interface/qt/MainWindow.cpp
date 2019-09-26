#include "MainWindow.h"
#include <AssignableWidget.h>

#include <QLabel>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_mainWidget = new QWidget;
    m_mainLayout = new QGridLayout;
    
    // Toolbar can't be dragged if it's not a direct child of MainWindow
    m_mainToolBar = new QToolBar;
    m_mainToolBar->setMovable(true);
    m_mainToolBar->setOrientation(Qt::Horizontal);
    m_mainToolBar->setFloatable(true);
    m_mainLayout->addWidget(m_mainToolBar);
    
    // Add toolbar buttons
    QToolButton *settingsButton = new QToolButton;
    settingsButton->setCheckable(true);
    settingsButton->setIcon(QIcon::fromTheme("configure"));
    m_mainToolBar->addWidget(settingsButton);
    
    AssignableWidget *assignableWidget = new AssignableWidget;
    m_mainLayout->addWidget(assignableWidget);
    
    m_mainWidget->setLayout(m_mainLayout);
    setCentralWidget(m_mainWidget);
}

MainWindow::~MainWindow() {
}
