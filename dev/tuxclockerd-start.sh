cd "$(dirname "$0")"

DBUS_SYSTEM_BUS_ADDRESS=unix:path=/tmp/tuxclocker-dbus-socket sudo -E LD_LIBRARY_PATH=../inst/lib ../inst/bin/tuxclockerd
