#include <QApplication>

#include "MainWindow.hpp"

#include <QDBusConnection>

int main(int argc, char **argv) {
	QApplication app(argc, argv);
	
	MainWindow mw;
	mw.show();
	
	QObject o;
	
	auto conn = QDBusConnection::sessionBus();
	conn.registerObject("/", &o);
	
	conn.registerService("org.tuxclockerqt");
	
	return app.exec();
}
