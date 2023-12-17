subdir('brushes')
subdir('dynamics')
subdir('environ')
subdir('gradients')
subdir('images')
subdir('interpreters')
subdir('palettes')
subdir('patterns')
subdir('tags')
subdir('tips')
subdir('tool-presets')


meson.add_install_script('sh', '-c',
  'mkdir -p $MESON_INSTALL_DESTDIR_PREFIX/@0@'.format(gimpdatadir / 'fonts')
)

release_conf = configuration_data()
release_conf.set('REVISION', get_option('revision'))

gimp_release = 'gimp-release'
configure_file(
  input : gimp_release + '.in',
  output: gimp_release,
  configuration: release_conf,
  install: true,
  install_dir: gimpdatadir,
)

# Fix iso-codes warning (Local-Native only)
if platform_windows and not meson.is_cross_build()
   install_data(
     join_paths(isocodes_location, 'iso_639.xml'),
     install_dir: join_paths(get_option('datadir'), 'xml/iso-codes')
)
endif