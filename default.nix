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
    rev = "2557227170cb94be12008bced6b9b3cfd93b8342";
    sha256 = "0gncf65hy11zwjf7mpz4k57ik15980mssykj2r1zd6gda48pw832";
    #rev = "002c2868118b2fa7d2e8db9d8a84a6b47dd261e5";
    #rev = "b0c39bfaeb1b5da20accd0d65dacfd5ac13a6225";
    #sha256 = "02jribnk9d9h1y01fbl9pafyz4vybfkma2mv73n7i707gfv6dk2r";
    #sha256 = "0a553i6mr9hxlfzvgidsn0ss0hbcr9jdiaknhv59bailalajn58i";
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
