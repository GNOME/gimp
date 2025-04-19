subdir('dynamics')
subdir('environ')
subdir('gradients')
subdir('interpreters')
subdir('palettes')
subdir('tags')
subdir('tips')
subdir('tool-presets')


#FIXME: https://gitlab.gnome.org/GNOME/gimp/-/issues/13761
if not platform_osx
  meson.add_install_script(python, '-c',
    'from pathlib import Path; from sys import argv; Path(argv[1]).mkdir(parents=True, exist_ok=True)', get_option('prefix') / gimpdatadir / 'fonts'
  )
else
  meson.add_install_script('sh', '-c',
    'mkdir -p $MESON_INSTALL_DESTDIR_PREFIX/@0@'.format(gimpdatadir / 'fonts')
  )
endif

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
