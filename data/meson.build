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
  'mkdir -p $MESON_INSTALL_DESTDIR_PREFIX/@0@'.format(ligmadatadir / 'fonts')
)

release_conf = configuration_data()
release_conf.set('REVISION', get_option('revision'))

ligma_release = 'ligma-release'
configure_file(
  input : ligma_release + '.in',
  output: ligma_release,
  configuration: release_conf,
  install: true,
  install_dir: ligmadatadir,
)
