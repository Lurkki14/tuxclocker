cd "$(dirname "$0")"

sudo dbus-run-session --config-file=dbusconf.conf \
	sudo -E DBUS_SYSTEM_BUS_ADDRESS=unix:path=/tmp/tuxclocker-dbus-socket \
	LD_LIBRARY_PATH=../inst/lib ../inst/bin/tuxclockerd
