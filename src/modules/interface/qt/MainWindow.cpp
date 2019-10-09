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
    
    m_mainStackedWidget = new QStackedWidget;
    
    m_assignableWidget = new AssignableWidget;
    m_mainStackedWidget->addWidget(m_assignableWidget);
    
    m_settingWidget = new QWidget;
    m_mainStackedWidget->addWidget(m_settingWidget);
    
    m_mainLayout->addWidget(m_mainStackedWidget);
    
    // Add toolbar buttons
    
    // Connect tool buttons to changing the active stacked widget
    QAction *activateSettings = new QAction;
    activateSettings->setCheckable(true);
    activateSettings->setIcon(QIcon::fromTheme("configure"));
    m_mainToolBar->addAction(activateSettings);
    
    QAction *activateEditor = new QAction;
    activateEditor->setCheckable(true);
    activateEditor->setIcon(QIcon::fromTheme("edit-entry"));
    m_mainToolBar->addAction(activateEditor);
    
    connect(activateSettings, &QAction::triggered, [=]() {
        changeActiveWidget(m_settingWidget, activateSettings);
    });
    
    /*connect(settingsButton, &QToolButton::toggled, [=](bool toggled) {
        if (toggled) {
            m_mainStackedWidget->setCurrentWidget(m_settingWidget);
        }
    });*/
    
    m_mainWidget->setLayout(m_mainLayout);
    setCentralWidget(m_mainWidget);
}

MainWindow::~MainWindow() {
}

void MainWindow::changeActiveWidget(QWidget *widget, const QAction *action) {
    
    // Replace this with list of actions that change widgets
   for (QAction *i_actions : m_mainToolBar->actions()) {
       
   }
   m_mainStackedWidget->setCurrentWidget(widget);
}
