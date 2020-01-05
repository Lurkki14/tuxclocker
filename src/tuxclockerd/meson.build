qt5 = import('qt5')
qt5_dep = dependency('qt5',
	modules : ['DBus'])
	
moc_files = qt5.preprocess(moc_headers : ['ReadableDynamicVariantAdaptor.h',
	'AssignableEnumAdaptor.h',
	'AssignableVariantRangeAdaptor.h'],
	dependencies : qt5_dep)
	
sources = ['main.cpp',
	'AssignableAdaptorFactory.cpp',
	'AssignableEnumAdaptor.cpp',
	'AssignableVariantRangeAdaptor.cpp',
	'ReadableAdaptorFactory.cpp',
	'ReadableDynamicVariantAdaptor.cpp']

executable('tuxclockerd',
	moc_files,
	sources,
	include_directories : incdir,
	dependencies : qt5_dep,
	link_with : libtuxclocker,
	install : true)

# DBus data directory

dbus_data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1')

dbus_system_services_dir = join_paths(dbus_data_dir, 'system.d')

install_data('org.tuxclocker.conf', install_dir : dbus_system_services_dir)
