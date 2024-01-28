incdir_tests = [ incdir, '../plugins/' ]

# https://github.com/mesonbuild/meson/issues/2518
# TODO: remove the if when this issue is fixed
if get_option('test')
	amdtests = executable('amdtest',
		'Tests.cpp', '../plugins/AMDUtils.cpp', '../plugins/Utils.cpp',
		# Find sample AMD files
		cpp_args : '-DPROJECT_ROOT="@0@"'.format(meson.source_root()),
		include_directories : [ incdir_tests, fplus_inc ],
		dependencies : [ libdrm_amdgpu, libdrm_dep, boost_dep ])

	test('AMD parsing', amdtests,
		protocol : 'exitcode')
endif
