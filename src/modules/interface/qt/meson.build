qt5 = import('qt5')
qt5_dep = dependency('qt5',
    modules : ['Widgets'])
    
moc_files = qt5.preprocess(moc_headers : [ 'MainWindow.h'],
    dependencies : qt5_dep)

sources = ['main.cpp',
    'MainWindow.cpp']
    
shared_library('qt', 
    sources,
    moc_files,
    include_directories : incdir,
    dependencies : qt5_dep)
