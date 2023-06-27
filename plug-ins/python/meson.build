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

if not stable
  plugins += [
    { 'name': 'test-dialog' },
  ]
endif

subdir('python-console')

foreach plugin : plugins
  name = plugin.get('name')
  srcs = plugin.get('srcs', [name + '.py'])

  install_data(srcs, install_dir: gimpplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')

  foreach src : srcs
    # Ugly trick to copy Python plug-ins into subfolders so that we can run GIMP
    # from the build directory without installing it.
    run_command(python, meson.project_source_root() / '.gitlab/cp-plug-in-subfolder.py',
                meson.current_source_dir() / src, meson.current_build_dir() / name, check: true)
  endforeach
endforeach


if python.found()
  python_config = configuration_data()
  python_config.set('PYTHON_PATH', python.full_path())

  configure_file(
    input : 'pygimp.interp.in',
    output: 'pygimp.interp',
    configuration: python_config,
    install: true,
    install_dir: gimpplugindir / 'interpreters',
  )
endif
