libdrm_amdgpu = cc.find_library('drm_amdgpu', required : false)
libdrm_dep = dependency('libdrm', required : false)

patterns_inc = include_directories('../include/deps/patterns/include/mpark')
fplus_inc = include_directories('../include/deps/FunctionalPlus/include')

if libdrm_dep.found() and libdrm_amdgpu.found()
	shared_library('amd', 'AMD.cpp', 'Utils.cpp',
		override_options : ['cpp_std=c++17'],
		include_directories : [incdir, patterns_inc, fplus_inc],
		dependencies : [ libdrm_amdgpu, libdrm_dep, boost_dep ],
		install_dir : get_option('libdir') / 'tuxclocker' / 'plugins',
		install : true,
                link_with : libtuxclocker)
endif

libnvml = cc.find_library('nvidia-ml', required : false)
libxext = cc.find_library('Xext', required : false)
libx = cc.find_library('X11', required : false)
libxnvctrl = cc.find_library('XNVCtrl', required : false)

nvidia_linux_libs = [libnvml, libx, libxext, libxnvctrl]

all_nvidia_linux_libs = true

foreach lib : nvidia_linux_libs
	if not lib.found()
		all_nvidia_linux_libs = false
		break
	endif
endforeach

if all_nvidia_linux_libs
	shared_library('nvidia', 'Nvidia.cpp', 'Utils.cpp',
		override_options : ['cpp_std=c++17'],
		include_directories : [incdir, patterns_inc, fplus_inc],
		dependencies : [nvidia_linux_libs, boost_dep],
		install_dir : get_option('libdir') / 'tuxclocker' / 'plugins',
		install : true,
		link_with : libtuxclocker)
endif

shared_library('cpu', 'CPU.cpp', 'Utils.cpp',
        include_directories : [incdir, fplus_inc],
        install_dir : get_option('libdir') / 'tuxclocker' / 'plugins',
        install : true,
        link_with : libtuxclocker)
