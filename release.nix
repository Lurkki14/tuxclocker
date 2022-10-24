with import <nixpkgs> {};
libsForQt5.callPackage ./default.nix {
  libXNVCtrl = linuxPackages.nvidia_x11.settings.libXNVCtrl;
  nvidia_x11 = linuxPackages.nvidia_x11;
  #boost = boost179;
  # TODO: I'd like to pin this, but specifying 1.79 gives a compile error even though the exact same hash is used if 'boost' is used
}
