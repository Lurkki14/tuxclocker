TuxClocker - A GUI overclocking utility for GNU/Linux
========================================
TuxClocker is a Qt5 overclocking tool. Currently supported cards are nvidia 600-series cards and newer, and AMD GPUs using the amdgpu driver until (not including) Radeon VII.

# Support
Matrix room: #tuxclocker:matrix.org [Direct Riot link](https://riot.im/app/#/room/#tuxclocker:matrix.org)

# Screenshots

![Imgur](https://i.imgur.com/fn8MoNj.png) ![Imgur](https://i.imgur.com/fuKIVW7.png) ![Imgur](https://i.imgur.com/cZCNzmN.png) ![Imgur](https://i.imgur.com/qkp2p7V.png) ![Imgur](https://i.imgur.com/TpmU8PD.png)

# Current features
- GPU monitoring (list and graph)
- Overclocking
- Overvolting
- Change power limit
- (AMD) pstate editing
- Fan mode selection
- Custom fan curve
- Provisional multi-GPU support
- Profiles

# Requirements (nvidia)
- NOTE: headers are usually included in a package named \*-dev, if they are separate
- nvidia-smi
- nvidia-settings
- libxnvctrl and headers (if not included in nvidia-settings)
- qt5base, x11extras and their headers
- [Coolbits](https://wiki.archlinux.org/index.php/NVIDIA/Tips_and_tricks#Enabling_overclocking) set to the value you want (31 for all functionality)

# Installation (nvidia)

### Compilation

NOTE: on some systems, qmake is linked to qt4-qmake. If qmake fails, run qmake-qt5 in place of qmake

```
git clone https://github.com/Lurkki14/tuxclocker
cd tuxclocker
qmake rojekti.pro
make
make install (installs into /opt/tuxclocker/bin)
```
### Arch Linux
#### AUR package
[https://aur.archlinux.org/packages/tuxclocker/](https://aur.archlinux.org/packages/tuxclocker/)

# Requirements (AMD)

- NOTE: headers are usually included in a package named \*-dev, if they are separate
- libdrm and headers
- libqt5x11extras5
- amdgpu.ppfeaturemask boot paramter set to the value you want. To view the current value, run 

```
printf "0x%08x\n" $(cat /sys/module/amdgpu/parameters/ppfeaturemask)
```

Example grub line (usually /etc/default/grub):	

```
GRUB_CMDLINE_LINUX_DEFAULT="quiet radeon.si_support=0 amdgpu.si_support=1 amdgpu.dpm=1 amdgpu.ppfeaturemask=0xffffffff"
```
	
After editing, update grub, usually 

```
sudo update-grub
```

# Installation (AMD)

### Compilation

NOTE: on some, systems, qmake is linked to qt4-qmake. If qmake fails, run qmake-qt5 in place of qmake

```
git clone https://github.com/Lurkki14/tuxclocker
cd tuxclocker
git checkout pstatetest
qmake rojekti.pro
make
make install (installs into /opt/tuxclocker/bin)
```
NOTE: to use fancurves on the AMD version, you need to run as root.
