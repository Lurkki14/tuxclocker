qt5 = import('qt5')
qt5_dep = dependency('qt5',
	modules : ['DBus'])
	
boost_dep = dependency('boost', modules : ['system'])
	
patterns_inc = include_directories('../include/deps/patterns/include/mpark')
	
moc_files = qt5.preprocess(moc_headers : ['Adaptors.hpp'],
	dependencies : qt5_dep)
	
sources = ['main.cpp']
	
executable('tuxclockerd',
	sources,
	moc_files,
	override_options : ['cpp_std=c++17'],
	include_directories : [incdir, patterns_inc],
	dependencies : [qt5_dep, boost_dep],
	link_with : libtuxclocker,
	install : true)
	
# DBus data directories
dbus_data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1')
dbus_system_services_dir = join_paths(dbus_data_dir, 'system.d')
install_data('org.tuxclocker.conf', install_dir : dbus_system_services_dir)

dbus_service_dir = join_paths(dbus_data_dir, 'system-services')

# Replace the binary path in dbus files
dbus_conf_data = configuration_data()
dbus_conf_data.set('tuxclocker_daemon_path',
	join_paths(join_paths(get_option('prefix'), get_option('bindir')),
		'tuxclockerd'))
		
configure_file(input : 'org.tuxclocker.service.in',
	output : 'org.tuxclocker.service',
	configuration : dbus_conf_data,
	install_dir : dbus_service_dir)


