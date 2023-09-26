project('tuxclocker', 'c', 'cpp',
	default_options : ['cpp_std=c++17'])

cc = meson.get_compiler('c')
cppc = meson.get_compiler('cpp')

incdir = include_directories('src/include', 'src/include/deps')

subdir('src')
