qt5 = import('qt5')
qt5_dep = dependency('qt5',
    modules : ['Widgets', 'Charts'])
    
moc_files = qt5.preprocess(moc_headers : [ 'MainWindow.h',
        'widgets/AbstractExpandableItemEditor.h',
        'widgets/AssignableWidget.h',
        'widgets/IntRangeEditor.h',
        'widgets/EnumEditor.h',
        'widgets/GraphEditor.h',
        'widgets/ReadableWidget.h',
        'widgets/ReadableBrowser.h',
        'widgets/ReadableDisplay.h',
        'widgets/ReadableGraphDisplay.h',
        'widgets/ReadableTreeView.h',
        'data/AssignableEditorDelegate.h',
        'data/AssignableManager.h',
        'data/ReadableMasterObservable.h',
        'data/ReadableObservable.h'],
        dependencies : qt5_dep)

sources = ['main.cpp',
    'MainWindow.cpp',
    'widgets/AssignableWidget.cpp',
    'widgets/AssignableParametrizationEditor.cpp',
    'widgets/IntRangeEditor.cpp',
    'widgets/EnumEditor.cpp',
    'widgets/DragChartView.cpp',
    'widgets/GraphEditor.cpp',
    'widgets/ReadableWidget.cpp',
    'widgets/ReadableBrowser.cpp',
    'widgets/ReadableDisplay.cpp',
    'widgets/ReadableGraphDisplay.cpp',
    'data/AssignableData.cpp',
    'data/AssignableEditorDelegate.cpp',
    'data/AssignableManager.cpp',
    'data/ReadableManager.cpp',
    'data/ReadableMasterObservable.cpp',
    'data/ReadableObservable.cpp',
    'data/ReadableObservableManager.cpp']
    
local_qt_incdirs = ['widgets', 'data']
    
shared_library('qt', 
    sources,
    moc_files,
    include_directories : [incdir, local_qt_incdirs],
    dependencies : qt5_dep,
    link_with : libtuxclocker,
    install_dir : get_option('libdir') / 'tuxclocker' / 'modules' / 'interface',
    install : true)
