#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow {
public:
	explicit MainWindow(QWidget *parent = nullptr);
protected:
	virtual void closeEvent(QCloseEvent *event);
private:
	void restoreGeometryFromCache(QWidget *widget);

	Q_OBJECT
};
