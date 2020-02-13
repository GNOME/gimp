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

custom_target('gimp-release',
  output: [ 'gimp-release' ],
  command: [
    'echo', ['[package]' + '\nrevision=@0@'.format(get_option('revision'))]
  ],
  capture: true,
  install: true,
  install_dir: gimpdatadir,
)
