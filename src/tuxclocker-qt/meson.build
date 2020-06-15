qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['DBus', 'Charts', 'Widgets'])

# signals2
boost_dep = dependency('boost')

moc_files = qt5.preprocess(moc_headers: ['MainWindow.hpp',
		'data/AssignableItem.hpp',
		'data/AssignableProxy.hpp',
		'data/DeviceModel.hpp',
		'data/DynamicReadableProxy.hpp',
		'widgets/DragChartView.hpp'],
	qresources : ['resources/resources.qrc'],
	dependencies: qt5_dep)

sources = ['main.cpp',
	'data/AssignableItem.cpp',
	'data/AssignableProxy.cpp',
	'data/DeviceModel.cpp',
	'data/DeviceModelDelegate.cpp',
	'data/DeviceProxyModel.cpp',
	'data/DynamicReadableProxy.cpp',
	'widgets/DeviceTreeView.cpp',
	'widgets/DragChartView.cpp',
	'widgets/EnumEditor.cpp',
	'widgets/FlagEditor.cpp',
	'MainWindow.cpp']

local_incdir = include_directories(['data',
	'widgets',
	'../include/deps/FunctionalPlus/include',
	'../include/deps/patterns/include/mpark'])
	
executable('tuxclocker-qt',
	moc_files,
	sources,
	override_options : ['cpp_std=c++17'],
	dependencies: qt5_dep,
	include_directories: [incdir, local_incdir],
	install: true)
