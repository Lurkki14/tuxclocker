{ lib
, stdenv
, boost
, cmake
, cudatoolkit
, git
, fetchFromGitHub
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
    rev = "8460bce332e399a1e9bb2eaff128ae71ce9d2f6a";
    hash = "sha256-ez8NMJ5Lrx0X2xBj2WE6eG7xDoC16y5IK8qBdSFkm/M=";
  };

  # meson 0.57 should fix having to have these
  BOOST_INCLUDEDIR = "${lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${lib.getLib boost}/lib";

  preConfigure = ''
    NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -I${libXNVCtrl}/include"
    NIX_LDFLAGS="$NIX_LDFLAGS -L${libXNVCtrl}/lib"
  '';

  nativeBuildInputs = [
    pkg-config
  ];

  buildInputs = [
    boost
    cudatoolkit
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
