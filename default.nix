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
    rev = "91b68d5436f067da15b6c47c75ed4758161860f4";
    hash = "sha256-J8qWQO4n1C4MyZN73EBDP7wvYktf2c8sDTrIObjhD20=";
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
