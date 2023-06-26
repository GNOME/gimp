subdir('lang')

source_splash=meson.project_source_root() + '/data/images/gimp-splash.png'
installsplash = custom_target('installsplash-devel.bmp',
  input : [ 'splash2installer.py' ],
  depend_files: [ source_splash ],
  output: [ 'installsplash-devel.bmp', ],
  command: [ gimpmain_exe, '-nidfs', source_splash,
             '--batch-interpreter', 'python-fu-eval',
             '-b', '-', '--quit'],
  feed: true,
  build_by_default: true
)
