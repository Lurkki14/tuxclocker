qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['DBus', 'Charts', 'Widgets'])

moc_files = qt5.preprocess(moc_headers: ['MainWindow.hpp',
		'data/AssignableItem.hpp'],
	dependencies: qt5_dep)

sources = ['main.cpp',
	'data/DeviceModel.cpp',
	'data/DeviceModelDelegate.cpp',
	'MainWindow.cpp']

local_incdir = include_directories(['data',
	'widgets',
	'../include/deps/patterns/include/mpark'])
	
executable('tuxclocker-qt',
	moc_files,
	sources,
	dependencies: qt5_dep,
	include_directories: [incdir, local_incdir],
	install: true)
