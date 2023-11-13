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
    rev = "b838a1f5b117dc2ec33acbeef2b906ed95f10b1f";
    hash = "sha256-1DpKKlFdN0hZ7hL9KwnsJMRoAWxz5BS5iNtHKqnjINk=";
  };

  # meson 0.57 should fix having to have these
  BOOST_INCLUDEDIR = "${lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${lib.getLib boost}/lib";

  preConfigure = ''
    NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -I${libXNVCtrl}/include"
    NIX_LDFLAGS="$NIX_LDFLAGS -L${libXNVCtrl}/lib"
  '';

  nativeBuildInputs = [
    git
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
