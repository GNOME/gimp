if not have_python
  subdir_done()
endif

plugins = [
  { 'name': 'colorxhtml' },
  { 'name': 'file-openraster' },
  { 'name': 'foggify' },
  { 'name': 'gradients-save-as-css' },
  { 'name': 'palette-offset' },
  { 'name': 'palette-sort' },
  { 'name': 'palette-to-gradient' },
  { 'name': 'py-slice' },
  { 'name': 'python-eval' },
  { 'name': 'spyro-plus' },

  # { 'name': 'histogram-export' },
]

if not stable
  plugins += [
    { 'name': 'benchmark-foreground-extract' }
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
  python_config = configuration_data()
  python_config.set('PYTHON_PATH', python.path())

  configure_file(
    input : 'pygimp.interp.in',
    output: 'pygimp.interp',
    configuration: python_config,
    install: true,
    install_dir: gimpplugindir / 'interpreters',
  )
endif
