# Version string not used when building only plugins
need_version = get_option('gui') or get_option('daemon')

if need_version
	# Version information
	tuxclocker_version = '1.4.0'
	git_commit = run_command('git', 'describe', '--always', '--dirty').stdout().strip()
	version_string = '@0@, git commit @1@'.format(tuxclocker_version, git_commit)
	# Used in preprocessor
	version_string_def = '-DTUXCLOCKER_VERSION_STRING="@0@"'.format(version_string)
endif

# OpenSSL is used for hash functions
openssl_dep = dependency('openssl')

boost_dep = dependency('boost',
	modules : ['system',
		'filesystem'])

# Compile time definition for module path
plugin_path_def_template = '-DTC_PLUGIN_PATH="@0@/@1@/tuxclocker/plugins"'
plugin_path_def = plugin_path_def_template.format(get_option('prefix'), get_option('libdir'))

# Compile time definition for locale path
locale_path_def_template = '-DTUXCLOCKER_LOCALE_PATH="@0@/@1@/locale"'
locale_path_def = locale_path_def_template.format(get_option('prefix'), get_option('datadir'))

# Define libtuxclocker target here since others depend on it
libtuxclocker = shared_library('tuxclocker',
	['lib/Crypto.cpp',
	 'lib/Plugin.cpp'],
	override_options : ['cpp_std=c++17'],
	include_directories : incdir,
	dependencies : [boost_dep, openssl_dep],
	cpp_args : plugin_path_def,
	install : true)

if get_option('plugins')
	subdir('plugins')
endif

if get_option('library')
	subdir('lib')
endif

if get_option('daemon')
	subdir('tuxclockerd')
endif

if get_option('gui')
	subdir('tuxclocker-qt')
endif

if get_option('cli')
	subdir('tuxclocker-cli')
endif
