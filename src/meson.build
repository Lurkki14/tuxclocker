# Define libtuxclocker target here since others depend on it

libtuxclocker_posix_sources = ['lib/posix/module.c']

libtuxclocker_posix_libs = [cc.find_library('dl')]

# Posix library uses runpath for specifying module directory root

posix_rpath = get_option('libdir' ) + '/tuxclocker/modules'

libtuxclocker = shared_library('libtuxclocker',
	['lib/tc_assignable.c',
	 'lib/tc_module.c',
	 libtuxclocker_posix_sources],
	include_directories : incdir,
	dependencies : libtuxclocker_posix_libs,
	install_rpath : posix_rpath,
	link_args : '-Wl,--enable-new-dtags')

subdir('modules')
subdir('main')
subdir('lib')
