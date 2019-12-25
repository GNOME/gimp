if have_python

plugins = [
  { 'name': 'colorxhtml' },
  { 'name': 'file-openraster' },
  { 'name': 'foggify' },
  { 'name': 'gradients-save-as-css' },
  { 'name': 'palette-offset' },
  { 'name': 'palette-to-gradient' },
  { 'name': 'py-slice' },
  { 'name': 'spyro-plus' },

  # { 'name': 'histogram-export' },
  # { 'name': 'palette-sort' },
  # { 'name': 'python-eval' },
]

if not stable
  plugins += [
    # { 'name': 'benchmark-foreground-extract', },
    # { 'name': 'clothify', },
    # { 'name': 'shadow_bevel', },
    # { 'name': 'sphere', },
    # { 'name': 'whirlpinch', },
  ]
endif

subdir('python-console')

foreach plugin : plugins
  name = plugin.get('name')
  srcs = plugin.get('srcs', name + '.py')

  install_data(srcs, install_dir: gimpplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')
endforeach


if python.found()
  custom_target('pygimp.interp',
    output: 'pygimp.interp',
    command: [ 'echo',
      'python=@0@\n'          .format(python.path()),
      'python3=@0@\n'         .format(python.path()),
      '/usr/bin/python=@0@\n' .format(python.path()),
      ':Python:E::py::python3:'
    ],
    capture: true,
    install: true,
    install_dir: gimpplugindir / 'interpreters',
  )
endif

endif
