# Define libtuxclocker target here since others depend on it

# OpenSSL is used for hash functions
openssl_dep = dependency('openssl')
# Jansson is used for module database
jansson_dep = dependency('jansson')

# For posix only
dl = cppc.find_library('dl')

boost_dep = dependency('boost',
	modules : ['system',
		'filesystem'])

libtuxclocker_posix_sources = ['lib/posix/module.c',
    'lib/posix/filesystem.c']

libtuxclocker_posix_libs = [cc.find_library('dl')]

# Compile time definition for module path
plugin_path_def_template = '-DTC_PLUGIN_PATH="@0@/@1@/tuxclocker/plugins"'
plugin_path_def = plugin_path_def_template.format(get_option('prefix'), get_option('libdir'))

#libtuxclocker = shared_library('libtuxclocker',
#	['lib/tc_assignable.c',
#	 'lib/tc_module.c',
#	 'lib/tc_readable.c',
#	 'lib/tc_common.c',
#	 libtuxclocker_posix_sources],
#	include_directories : incdir,
#	dependencies : [libtuxclocker_posix_libs, openssl_dep, jansson_dep],
#	c_args : module_path_def,
#	install : true)

libtuxclocker = shared_library('tuxclocker',
	['lib/Crypto.cpp',
	 'lib/Plugin.cpp'],
	override_options : ['cpp_std=c++17'],
	include_directories : incdir,
	dependencies : [boost_dep, dl, openssl_dep],
	cpp_args : plugin_path_def,
	install : true)


if get_option('plugins')
	subdir('plugins')
endif

subdir('main')

if get_option('library')
	subdir('lib')
endif

if get_option('daemon')
	subdir('tuxclockerd')
endif

if get_option('gui')
	subdir('tuxclocker-qt')
endif
