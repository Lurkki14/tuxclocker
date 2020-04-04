# Installation

## Dependencies

boost-system, boost-filesystem, qtdbus

## Meson options

```
--prefix=<path> (install location prefix, usually '/usr')
-Dplugins=<true/false>
-Ddaemon=<true/false> (builds and installs 'tuxclockerd' binary/daemon)
```

```
git clone https://github.com/Lurkki14/tuxclocker
cd tuxclocker
git checkout cpplib
git submodule init
git submodule update
meson build <meson options>
cd build
ninja && sudo ninja install
```
