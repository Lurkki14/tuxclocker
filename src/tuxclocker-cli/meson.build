install_dir = join_paths(get_option('prefix'), get_option('bindir'))

meson.add_install_script(
	'call-cabal.sh', '--installdir=@0@'.format(install_dir))
