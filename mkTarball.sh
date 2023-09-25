#!/usr/bin/env sh

storePaths=($(nix-store -qR $(nix-build release.nix)))
cp -n result/bin/{.tuxclockerd-wrapped,.tuxclocker-qt-wrapped} .
libPaths=( "${storePaths[@]/%/\/lib\/}" )
<<<<<<< HEAD
<<<<<<< HEAD
libPaths=( "${libPaths[@]/#/.}" )
libPathsColonSep=$(echo ${libPaths[@]} | sed 's/ /:/g')
glibPath=$(nix-build '<nixpkgs>' -A glibc --no-out-link)
# TODO: don't hardcode version!
qtPluginPath=.$(nix-build '<nixpkgs>' -A libsForQt5.qt5.qtbase --no-out-link)/lib/qt-5.15.9/plugins/
tuxclockerPluginPath=.$(nix-build release.nix)/lib/tuxclocker/plugins/
chmod 777 ./.tuxclockerd-wrapped ./.tuxclocker-qt-wrapped
patchelf --set-rpath \.$glibPath/lib ./.tuxclockerd-wrapped ./.tuxclocker-qt-wrapped
patchelf --set-interpreter \.$glibPath"/lib/ld-linux-x86-64.so.2" ./.tuxclockerd-wrapped ./.tuxclocker-qt-wrapped

echo "
export DBUS_SYSTEM_BUS_ADDRESS='unix:path=/tmp/tuxclocker-dbus-socket'
export LD_LIBRARY_PATH=\"${libPathsColonSep[@]}\"
export QT_PLUGIN_PATH=\"$qtPluginPath\"
export TUXCLOCKER_PLUGIN_PATH=\"$tuxclockerPluginPath\"
sudo -E dbus-run-session --config-file=dev/dbusconf.conf \
sudo -E LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH\" ./.tuxclockerd-wrapped & \
(unset LD_LIBRARY_PATH; sleep 2) && ./.tuxclocker-qt-wrapped; \
unset LD_LIBRARY_PATH && \
sudo kill \$(pidof .tuxclockerd-wrapped)
" > run.sh
chmod +x run.sh
tar cavf tuxclocker.tar ${storePaths[@]} ./.tuxclocker-qt-wrapped ./.tuxclockerd-wrapped ./run.sh ./dev/dbusconf.conf
=======
#libPaths=( "${libPaths[@]/" "/\:}" )
=======
>>>>>>> 641bf86 (squash: working ld paths)
libPaths=( "${libPaths[@]/#/.}" )
libPathsColonSep=$(echo ${libPaths[@]} | sed 's/ /:/g')
glibPath=$(nix-build '<nixpkgs>' -A glibc --no-out-link)
chmod 777 ./.tuxclockerd-wrapped ./.tuxclocker-qt-wrapped
patchelf --set-rpath \.$glibPath/lib ./.tuxclockerd-wrapped ./.tuxclocker-qt-wrapped
patchelf --set-interpreter \.$glibPath/lib/ld-linux-x86-64.so.2 ./.tuxclockerd-wrapped ./.tuxclocker-qt-wrapped

echo "
export DBUS_SYSTEM_BUS_ADDRESS='unix:path=/tmp/tuxclocker-dbus-socket'
export LD_LIBRARY_PATH=\"${libPathsColonSep[@]}\"
sudo -E LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH\" dbus-run-session --config-file=dev/dbusconf.conf ./.tuxclockerd-wrapped & ./.tuxclocker-qt-wrapped
" > run.sh
chmod +x run.sh
<<<<<<< HEAD
#tar cavf foo.tar ${storePaths[@]} ./.tuxclocker-qt-wrapped ./.tuxclockerd-wrapped ./run.sh ./dev/dbusconf.conf
>>>>>>> 397cfb9 (squash: non-nix-bundle)
=======
tar cavf foo.tar ${storePaths[@]} ./.tuxclocker-qt-wrapped ./.tuxclockerd-wrapped ./run.sh ./dev/dbusconf.conf
>>>>>>> 641bf86 (squash: working ld paths)
