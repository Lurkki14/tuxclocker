qt5 = import('qt5')
qt5_dep = dependency('qt5',
    modules : ['Widgets'])
    
moc_files = qt5.preprocess(moc_headers : [ 'MainWindow.h',
        'widgets/AssignableWidget.h',
        'widgets/AssignableEditor.h'],
    dependencies : qt5_dep)

sources = ['main.cpp',
    'MainWindow.cpp',
    'widgets/AssignableWidget.cpp',
    'widgets/AssignableEditor.cpp']
    
local_qt_incdirs = ['widgets']
    
shared_library('qt', 
    sources,
    moc_files,
    include_directories : [incdir, local_qt_incdirs],
    dependencies : qt5_dep,
    link_with : libtuxclocker,
    install_dir : get_option('libdir') / 'tuxclocker' / 'modules' / 'interface',
    install : true)
