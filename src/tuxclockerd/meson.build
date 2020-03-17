qt5 = import('qt5')
qt5_dep = dependency('qt5',
	modules : ['DBus'])
	
boost_dep = dependency('boost', modules : ['system'])
	
#moc_files = qt5.preprocess(moc_headers : #['ReadableDynamicVariantAdaptor.h',
#	'AssignableEnumAdaptor.h',
#	'AssignableVariantRangeAdaptor.h'],
#	dependencies : qt5_dep)
	
sources = ['main.cpp']
	
executable('tuxclockerd',
	sources,
	override_options : ['cpp_std=c++17'],
	include_directories : incdir,
	dependencies : [qt5_dep, boost_dep, dl],
	link_with : libtuxclocker,
	install : true)

# DBus data directory

dbus_data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1')

dbus_system_services_dir = join_paths(dbus_data_dir, 'system.d')

install_data('org.tuxclocker.conf', install_dir : dbus_system_services_dir)
