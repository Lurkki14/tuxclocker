libnvml = cc.find_library('nvidia-ml', required : false)
libx = cc.find_library('X11', required : false)
libxnvctrl = cc.find_library('XNVCtrl', required : false)

nvidia_linux_libs = [libnvml, libx, libxnvctrl]

all_nvidia_linux_libs = true

foreach lib : nvidia_linux_libs
	if not lib.found()
		all_nvidia_linux_libs = false
		break
	endif
endforeach


if all_nvidia_linux_libs
	shared_library('nvidia', 'nvidia_linux.c',
		include_directories : incdir,
		dependencies : nvidia_linux_libs,
		link_with : libtuxclocker,
		install_dir : get_option('libdir') / 'tuxclocker' / 'modules' / 'assignable',
		install : true)
endif
