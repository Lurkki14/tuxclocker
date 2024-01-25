patterns_inc = include_directories('../include/deps/patterns/include/mpark')

req_amd = get_option('require-amd')
libdrm_amdgpu = cc.find_library('drm_amdgpu', required : req_amd)
libdrm_dep = dependency('libdrm', required : req_amd)

python = import('python')
python_with_hwdata = python.find_installation('python3',
	modules : ['hwdata'],
	required : get_option('require-python-hwdata'))

if libdrm_dep.found() and libdrm_amdgpu.found()
	sources = ['AMD.cpp', 'AMDUtils.cpp', 'Utils.cpp']
	cpp_args = []
	deps = [ libdrm_amdgpu, libdrm_dep, boost_dep ]
	if (python_with_hwdata.found())
		hwdata_version_cmd = run_command('python3', '-c',
			'from importlib.metadata import version; print(version(\'hwdata\'))')
		hwdata_version = hwdata_version_cmd.stdout().strip()
		if (hwdata_version.version_compare('<2.4.1'))
			warning('python-hwdata 2.4.1 is recommended for PCI.get_subsystem')
		endif
		# Makes us link to libpython
		deps += python_with_hwdata.dependency(embed : true)
		cpp_args += '-DWITH_HWDATA'
		sources += 'HWData.cpp'
	endif
	shared_library('amd',
		sources,
		override_options : ['cpp_std=c++17'],
		include_directories : [incdir, patterns_inc, fplus_inc],
		dependencies : deps,
		install_dir : get_option('libdir') / 'tuxclocker' / 'plugins',
		install : true,
		cpp_args : cpp_args,
		link_with : libtuxclocker)
endif

req_nv = get_option('require-nvidia')
libnvml = cc.find_library('nvidia-ml', required : req_nv)
libxext = cc.find_library('Xext', required : req_nv)
libx = cc.find_library('X11', required : req_nv)
libxnvctrl = cc.find_library('XNVCtrl', required : req_nv)

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

if get_option('plugins-cpu')
	shared_library('cpu', 'CPU.cpp', 'Utils.cpp',
		include_directories : [incdir, fplus_inc],
		install_dir : get_option('libdir') / 'tuxclocker' / 'plugins',
		install : true,
		link_with : libtuxclocker)
endif
