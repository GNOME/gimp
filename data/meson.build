subdir('dynamics')
subdir('environ')
subdir('gradients')
subdir('interpreters')
subdir('palettes')
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
