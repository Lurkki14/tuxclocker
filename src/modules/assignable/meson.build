
libnvml = cc.find_library('nvidia-ml')
libx = cc.find_library('X11')
libxnvctrl = cc.find_library('XNVCtrl')

nvml_h = cc.has_header('nvml.h')

shared_library('libnvidia', 'nvidia_linux.c',
	include_directories : incdir,
	dependencies : [libnvml, libx, libxnvctrl],
	link_with : libtuxclocker)

