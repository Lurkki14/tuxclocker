# Define libtuxclocker target here since others depend on it

# OpenSSL is used for hash functions
openssl_dep = dependency('openssl')

libtuxclocker_posix_sources = ['lib/posix/module.c',
    'lib/posix/filesystem.c']

libtuxclocker_posix_libs = [cc.find_library('dl')]

# Compile time definition for module path
module_path_def_template = '-DTC_MODULE_PATH="@0@/@1@/tuxclocker/modules"'
module_path_def = module_path_def_template.format(get_option('prefix'), get_option('libdir'))

libtuxclocker = shared_library('libtuxclocker',
	['lib/tc_assignable.c',
	 'lib/tc_module.c',
	 'lib/tc_readable.c',
	 'lib/tc_common.c',
	 libtuxclocker_posix_sources],
	include_directories : incdir,
	dependencies : [libtuxclocker_posix_libs, openssl_dep],
	c_args : module_path_def,
	install : true)

subdir('modules')
subdir('main')

if get_option('library')
	subdir('lib')
endif

if get_option('daemon')
	subdir('tuxclockerd')
endif
