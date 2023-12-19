cd "$(dirname "$0")"

sudo dbus-run-session --config-file=dbusconf.conf \
	sudo -E DBUS_SYSTEM_BUS_ADDRESS=unix:path=/tmp/tuxclocker-dbus-socket \
	XDG_SESSION_TYPE=$XDG_SESSION_TYPE \
	LD_LIBRARY_PATH=../inst/lib ../inst/bin/tuxclockerd
