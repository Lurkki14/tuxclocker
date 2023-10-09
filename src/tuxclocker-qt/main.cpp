#include <libintl.h>
#include <QApplication>

#include "MainWindow.hpp"

int main(int argc, char **argv) {
	QApplication app(argc, argv);

	setlocale(LC_MESSAGES, "");
	bindtextdomain("tuxclocker", TUXCLOCKER_LOCALE_PATH);
	bind_textdomain_codeset("tuxclocker", "UTF-8");
	textdomain("tuxclocker");

	MainWindow mw;
	mw.show();

	return app.exec();
}
