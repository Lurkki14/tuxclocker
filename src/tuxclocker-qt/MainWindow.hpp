#pragma once

#include <QMainWindow>

class QSystemTrayIcon;

class MainWindow : public QMainWindow {
public:
	explicit MainWindow(QWidget *parent = nullptr);
	void setTrayIconEnabled(bool enable);
protected:
	virtual void closeEvent(QCloseEvent *event);
private:
	QMenu *createTrayMenu();
	void restoreGeometryFromCache(QWidget *widget);
	void saveStateToCache();
	void restoreWindow();
	QSystemTrayIcon *m_trayIcon;
	bool m_startMinimized = false;

	Q_OBJECT
};
