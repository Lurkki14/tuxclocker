with import <nixpkgs> {};
libsForQt5.callPackage ./default.nix {
  libXNVCtrl = linuxPackages.nvidia_x11.settings.libXNVCtrl;
  nvidia_x11 = linuxPackages.nvidia_x11;
}
