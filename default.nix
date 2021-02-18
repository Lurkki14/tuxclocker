{ lib
, stdenv
, boost
, cmake
, git
, fetchFromGitHub
, libdrm
, meson
, mkDerivation
, ninja
, pkg-config
}:

mkDerivation rec {
  pname = "tuxclocker";
  version = "0.1";

  src = fetchFromGitHub {
    fetchSubmodules = true;
    owner = "Lurkki14";
    repo = "tuxclocker";
    rev = "b0c39bfaeb1b5da20accd0d65dacfd5ac13a6225";
    sha256 = "02jribnk9d9h1y01fbl9pafyz4vybfkma2mv73n7i707gfv6dk2r";
  };

  # meson 0.57 should fix having to have these
  BOOST_INCLUDEDIR = "${lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${lib.getLib boost}/lib";

  nativeBuildInputs = [
    pkg-config
  ];

  buildInputs = [
    boost
    libdrm
    meson
    ninja
    pkg-config
  ];
}
