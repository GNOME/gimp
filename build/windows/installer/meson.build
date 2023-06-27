subdir('lang')

source_splash=meson.project_source_root() + '/data/images/gimp-splash.png'

# make GIMP runnable without being installed.
env=environment()

menu_paths=meson.project_build_root() / 'menus:' + meson.project_source_root() / 'menus'
env.set('GIMP_TESTING_MENUS_PATH', menu_paths)

env.set('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'plug-ins:')
env.append('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'plug-ins/python')
env.append('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'plug-ins/common/test-plug-ins/')

env.append('GI_TYPELIB_PATH', meson.project_build_root() / 'libgimp')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimp')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpbase')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpcolor')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpconfig')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpmath')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpmodule')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpthumb')
env.append('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpwidgets')

installsplash = custom_target('installsplash-devel.bmp',
  input : [ 'splash2installer.py' ],
  depend_files: [ source_splash ],
  output: [ 'installsplash-devel.bmp', ],
  command: [ gimpmain_exe, '-nidfs', source_splash,
             '--batch-interpreter', 'python-fu-eval',
             '-b', '-', '--quit'],
  feed: true,
  build_by_default: true,
  env: env
)
