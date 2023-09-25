let
  tuxclocker = import ./release.nix;
  sources = import ./npins;
  dbusConf = ./dev/dbusconf.conf;
  pkgs =
    if (builtins.pathExists ./npins)
    then import sources.nixpkgs {}
    else import <nixpkgs> {};
  # Rename dbus binary so we don't kill host dbus when exiting
  dbus = pkgs.dbus.overrideAttrs {
    postInstall = ''
      mv $out/bin/dbus-daemon $out/bin/tuxclocker-dbus
    '';
  };
in
  pkgs.writeScriptBin "tuxclocker" ''
    export DBUS_SYSTEM_BUS_ADDRESS=unix:path=/tmp/tuxclocker-dbus-socket
    export LD_LIBRARY_PATH=""
    sudo ${dbus}/bin/tuxclocker-dbus --config-file=${dbusConf} &&
    sudo -E ${tuxclocker}/bin/tuxclockerd & ${tuxclocker}/bin/tuxclocker-qt;
    sudo kill $(pidof tuxclockerd); sudo kill $(pidof tuxclocker-dbus)
  ''
