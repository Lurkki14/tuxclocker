# Define libtuxclocker target here since others depend on it

libtuxclocker_posix_sources = ['lib/posix/module.c']

libtuxclocker_posix_libs = [cc.find_library('dl')]

libtuxclocker = shared_library('libtuxclocker',
	['lib/tc_assignable.c',
	 'lib/tc_module.c',
	 libtuxclocker_posix_sources],
	include_directories : incdir,
	dependencies : libtuxclocker_posix_libs)

subdir('modules')
subdir('main')
subdir('lib')
