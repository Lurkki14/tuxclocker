cd "$(dirname "$0")"

DBUS_VERBOSE=1 sudo dbus-daemon --config-file=dbusconf.conf --nofork
