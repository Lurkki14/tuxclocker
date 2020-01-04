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
