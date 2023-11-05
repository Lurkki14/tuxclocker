#include <libintl.h>
#include <QApplication>
#include <QCommandLineParser>

#include "MainWindow.hpp"

int main(int argc, char **argv) {
	QApplication app(argc, argv);
	app.setApplicationVersion(TUXCLOCKER_VERSION_STRING);

	QCommandLineParser parser;
	parser.addVersionOption();
	parser.process(app);

	setlocale(LC_MESSAGES, "");
	bindtextdomain("tuxclocker", TUXCLOCKER_LOCALE_PATH);
	bind_textdomain_codeset("tuxclocker", "UTF-8");
	textdomain("tuxclocker");

	MainWindow mw;
	mw.show();

	return app.exec();
}
