if not have_python
  subdir_done()
endif

plugins = [
  { 'name': 'colorxhtml' },
  { 'name': 'file-openraster' },
  { 'name': 'foggify' },
  { 'name': 'gradients-save-as-css' },
  { 'name': 'histogram-export' },
  { 'name': 'palette-offset' },
  { 'name': 'palette-sort' },
  { 'name': 'palette-to-gradient' },
  { 'name': 'python-eval' },
  { 'name': 'spyro-plus' },
]

subdir('python-console')

foreach plugin : plugins
  name = plugin.get('name')
  srcs = plugin.get('srcs', name + '.py')

  install_data(srcs, install_dir: ligmaplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')
endforeach


if python.found()
  python_config = configuration_data()
  python_config.set('PYTHON_PATH', python.full_path())

  configure_file(
    input : 'pyligma.interp.in',
    output: 'pyligma.interp',
    configuration: python_config,
    install: true,
    install_dir: ligmaplugindir / 'interpreters',
  )
endif
