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
    
    m_readableWidget = new ReadableWidget;
    m_mainStackedWidget->addWidget(m_readableWidget);
    
    m_settingWidget = new QWidget;
    m_mainStackedWidget->addWidget(m_settingWidget);
    
    m_mainLayout->addWidget(m_mainStackedWidget);
    
    // Add toolbar buttons
    
    // Connect tool buttons to changing the active stacked widget
    QAction *activateEditor = new QAction;
    setupWidgetTriggerAction(activateEditor, m_assignableWidget, "edit-entry");
    
    QAction *activateSettings = new QAction;
    setupWidgetTriggerAction(activateSettings, m_settingWidget, "configure");
    
    QAction *activateViewer = new QAction;
    setupWidgetTriggerAction(activateViewer, m_readableWidget, "document-properties");
    
    // Set editor as default widget
    changeActiveWidget(m_assignableWidget, activateEditor);
    
    m_mainWidget->setLayout(m_mainLayout);
    setCentralWidget(m_mainWidget);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupWidgetTriggerAction(QAction *action, QWidget *widget, const QString &iconName) {
    // Add action to triggers
    m_widgetSwitchTriggers.append(action);
    
    action->setCheckable(true);
    action->setIcon(QIcon::fromTheme(iconName));
    m_mainToolBar->addAction(action);
    
    // Connect action and widget switch
    connect(action, &QAction::triggered, [=]() {
        changeActiveWidget(widget, action);
    });
}

void MainWindow::changeActiveWidget(QWidget *widget, QAction *action) {
    action->setChecked(true);
    
    for (QAction *i_action : m_widgetSwitchTriggers) {
        if (i_action != action) {
            // Uncheck other switch actions
            i_action->setChecked(false);
        }
    }
    m_mainStackedWidget->setCurrentWidget(widget);
}
