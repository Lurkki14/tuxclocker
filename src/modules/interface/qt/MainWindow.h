#pragma once

#include <AssignableWidget.h>

#include <QMainWindow>
#include <QLayout>
#include <QToolBar>
#include <QToolButton>
#include <QStackedWidget>
#include <QMap>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    QGridLayout *m_mainLayout;
    QStatusBar *m_mainStatusBar;
    QWidget *m_mainWidget;
    
    QToolBar *m_mainToolBar;
    
    // Stacked widgets for the main view
    QStackedWidget *m_mainStackedWidget;
    AssignableWidget *m_assignableWidget;
    QWidget *m_settingWidget;

    // List of widget switch triggers so we know which ones to uncheck
    QVector <QAction*>  m_widgetSwitchTriggers;
    // Setup connection between action and widget
    void setupWidgetTriggerAction(QAction *action, QWidget *widget, const QString &iconName);
    // Change current stacked widget according to the action that was triggered
    void changeActiveWidget(QWidget *widget, QAction *action);
};
