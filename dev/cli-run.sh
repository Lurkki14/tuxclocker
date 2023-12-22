cd "$(dirname "$0")"

DBUS_SYSTEM_BUS_ADDRESS=unix:path=/tmp/tuxclocker-dbus-socket ../inst/bin/tuxclocker
