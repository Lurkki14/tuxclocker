#pragma once

#include <QMainWindow>
#include <QLayout>
#include <QToolBar>
#include <QToolButton>
#include <QStackedWidget>

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
    QVector <QWidget*> m_mainStackedWidgetWidgets;
};
