qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['DBus', 'Charts', 'Widgets'])

moc_files = qt5.preprocess(moc_headers: ['MainWindow.hpp'],
	dependencies: qt5_dep)

sources = ['main.cpp',
	'MainWindow.cpp']

executable('tuxclocker-qt',
	moc_files,
	sources,
	dependencies: qt5_dep,
	include_directories: [incdir],
	install: true)
