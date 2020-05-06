#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
	Q_OBJECT
};
