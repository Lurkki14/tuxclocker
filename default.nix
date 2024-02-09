{ lib
, stdenv
, boost
, cmake
, fetchFromGitHub
, git
, haskellPackages
, libdrm
, libX11
, libXext
, libXNVCtrl # this is supposed to work, but with the qt5.callPackages thing doesn't?
, meson
, mkDerivation
, ninja
, nvidia_x11
, pkg-config
, qtbase
, qtcharts
}:

mkDerivation rec {
  pname = "tuxclocker";
  version = "0.1";

  src = fetchFromGitHub {
    fetchSubmodules = true;
    owner = "Lurkki14";
    repo = "tuxclocker";
    rev = "edb7a8d19e7649f3dc423763811f66165268da48";
    hash = "sha256-Rxvv92q+9GBP7X1OmFBS4gYKREhbTw++h8BP4mriick=";
  };

  # meson 0.57 should fix having to have these
  BOOST_INCLUDEDIR = "${lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${lib.getLib boost}/lib";

  preConfigure = ''
    NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -I${libXNVCtrl}/include"
    NIX_LDFLAGS="$NIX_LDFLAGS -L${libXNVCtrl}/lib"
  '';

  nativeBuildInputs = [
    (haskellPackages.ghcWithPackages (p: with p; [ dbus ]))
    git
    pkg-config
  ];

  buildInputs = [
    boost
    libdrm
    libXext
    libX11
    libXNVCtrl
    meson
    ninja
    nvidia_x11
    pkg-config
    qtbase
    qtcharts
  ];
}
