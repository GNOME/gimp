if have_python

subdir('doc')
subdir('plug-ins')

pwd = meson.current_source_dir()
pygtk_generator = generator(pygobjectcodegen,
  arguments: [
      '--override', join_paths(pwd, '@BASENAME@.override'),
      '--register', join_paths(pygtk_defsdir, 'gdk-types.defs'),
      '--register', join_paths(pygtk_defsdir, 'gtk-types.defs'),
      '--register', join_paths(pwd, 'gimp-types.defs'),
      '--register', join_paths(pwd, 'gimpcolor-types.defs'),
      '--register', join_paths(pwd, 'gimpenums-types.defs'),
      '--prefix', '@BASENAME@',
      '@INPUT@',
  ],
  output: '@BASENAME@.c',
  capture: true,
)


python_so_libs = [
  [ 'gimp',
    [
      'gimpmodule.c',
      'pygimp-display.c',
      'pygimp-drawable.c',
      'pygimp-image.c',
      'pygimp-item.c',
      'pygimp-parasite.c',
      'pygimp-pdb.c',
      'pygimp-tile.c',
      'pygimp-vectors.c',
    ],
    [ pycairo, cairo, gtk2, ],
    [
      libgimp,
      libgimpbase,
      libgimpcolor,
      libgimpui,
    ],
  ],
  [ '_gimpenums',
    [ 'gimpenumsmodule.c', ],
    [ gobject, cairo, gtk2, ],
    [
      libgimp,libgimpbase
    ],
  ],
  [ '_gimpui',
    [ 'gimpuimodule.c',    pygtk_generator.process('gimpui.defs'), ],
    [ pygtk2, pycairo, gtk2, ],
    [
      libgimp,
      libgimpbase,
      libgimpcolor,
      libgimpconfig,
      libgimpui,
      libgimpwidgets
    ],
  ],
  [ 'gimpcolor',
    [ 'gimpcolormodule.c', 'pygimp-colors.c', ],
    [cairo],
    [ libgimpcolor, ],
  ],
  [ 'gimpthumb',
    [ 'gimpthumbmodule.c', pygtk_generator.process('gimpthumb.defs'), ],
    [],
    [ libgimpthumb, ],
  ],
]

foreach pylib : python_so_libs
  shared_library(pylib[0],
    pylib[1],
    include_directories: [ rootInclude, ],
    dependencies: pylib[2] + [
      libpython2, pygobject,
      gdk_pixbuf, gegl,
    ],
    link_with: pylib[3],
    name_prefix: '',
    install: true,
    install_dir: join_paths(gimpplugindir, 'python'),
  )
endforeach



install_data([
    'gimpenums.py',
    'gimpfu.py',
    'gimpplugin.py',
    'gimpshelf.py',
    'gimpui.py',
    'pygimp-logo.png',
  ],
  install_dir: join_paths(gimpplugindir, 'python'),
)

path_separator = platform_win32 ? ';' : ':'

custom_target('pygimp.env',
  input : [],
  output: [ 'pygimp.env' ],
  command: [
    'echo', path_separator + ' PYTHONPATH=${gimp_plug_in_dir}/python'
  ],
  capture: true,
  install: true,
  install_dir: join_paths(gimpplugindir, 'environ'),
)
custom_target('pygimp.interp',
  input : [],
  output: [ 'pygimp.interp' ],
  command: [
    'echo', 'python='          + python2.path()
    + '\n'+ '/usr/bin/python=' + python2.path()
    + '\n'+ ':Python:E::py::python:'
  ],
  capture: true,
  install: true,
  install_dir: join_paths(gimpplugindir, 'interpreters'),
)

endif
