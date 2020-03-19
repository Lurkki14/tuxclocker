libdrm_amdgpu = cppc.find_library('libdrm_amdgpu', required : false)
libdrm_dep = dependency('libdrm', required : false)

if libdrm_dep.found() and libdrm_amdgpu.found()
	shared_library('amd', 'AMD.cpp',
		override_options : ['cpp_std=c++17'],
		include_directories : incdir,
		dependencies : [libdrm_amdgpu, libdrm_dep],
		install_dir : get_option('libdir') / 'tuxclocker' / 'plugins',
		install : true)
endif
