if have_python

subdir('doc')
subdir('plug-ins')

# pwd = meson.current_source_dir()
# pygtk_generator = generator(pygobjectcodegen,
#   arguments: [
#       '--override', pwd / '@BASENAME@.override',
#       '--register', pygtk_defsdir / 'gdk-types.defs',
#       '--register', pygtk_defsdir / 'gtk-types.defs',
#       '--register', pwd / 'gimp-types.defs',
#       '--register', pwd / 'gimpcolor-types.defs',
#       '--register', pwd / 'gimpenums-types.defs',
#       '--prefix', '@BASENAME@',
#       '@INPUT@',
#   ],
#   output: '@BASENAME@.c',
#   capture: true,
# )


python_so_libs = [
   {
    'name': 'gimp',
    'srcs': [
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
    'deps': [
      py2cairo,
      cairo,
      gtk3,
    ],
    'link': [
      libgimp,
      libgimpbase,
      libgimpcolor,
      libgimpui,
    ],
  },
  { 'name': '_gimpenums',
    'srcs': 'gimpenumsmodule.c',
    'deps': [
      gobject,
      cairo,
      gtk3,
    ],
    'link': [
      libgimp,
      libgimpbase,
    ],
  },
  { 'name': '_gimpui',
    'srcs': 'gimpuimodule.c',
    'deps': [
      py2cairo,
      gtk3,
    ],
    'link': [
      libgimp,
      libgimpbase,
      libgimpcolor,
      libgimpconfig,
      libgimpui,
      libgimpwidgets
    ],
  },
  { 'name': 'gimpcolor',
    'srcs': [
      'gimpcolormodule.c',
      'pygimp-colors.c',
    ],
    'deps': [
      cairo
    ],
    'link': [
      libgimpcolor,
    ],
  },
  { 'name': 'gimpthumb',
    'srcs': 'gimpthumbmodule.c',
    'deps': [],
    'link': [
      libgimpthumb,
    ],
  },
]

foreach pylib : python_so_libs
  name = pylib.get('name')
  srcs = pylib.get('srcs')
  deps = pylib.get('deps')
  link = pylib.get('link')

  shared_library(name,
    srcs,
    include_directories: [ rootInclude, ],
    dependencies: deps + [
      libpython2, py2gobject,
      gdk_pixbuf, gegl,
    ],
    link_with: link,
    c_args: '-DGIMP_DEPRECATED_REPLACE_NEW_API',
    name_prefix: '',
    install: true,
    install_dir: gimpplugindir / 'python',
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
  install_dir: gimpplugindir / 'python',
)

path_separator = platform_windows ? ';' : ':'

custom_target('pygimp.env',
  output: [ 'pygimp.env' ],
  command: [
    'echo', path_separator + ' PYTHONPATH=${gimp_plug_in_dir}/python'
  ],
  capture: true,
  install: true,
  install_dir: gimpplugindir / 'environ',
)
custom_target('pygimp.interp',
  output: [ 'pygimp.interp' ],
  command: [ 'echo',
    'python='          + python2.path() + '\n' +
    'python2='         + python2.path() + '\n' +
    '/usr/bin/python=' + python2.path() + '\n' +
    ':Python:E::py::python2:'
  ],
  capture: true,
  install: true,
  install_dir: gimpplugindir / 'interpreters',
)

endif
