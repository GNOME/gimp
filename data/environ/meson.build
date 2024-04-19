environ_config = configuration_data()

if platform_windows and relocatable_bundle
  environ_config.set('BIN_PATH', 'PATH=${gimp_installation_dir}\\bin')
else
  environ_config.set('BIN_PATH', '')
endif

configure_file(
  input : 'default.env.in',
  output: 'default.env',
  configuration: environ_config,
  install: true,
  install_dir: gimpplugindir / 'environ',
)


if not meson.is_cross_build() and python.found()
  install_data(
    'python.env',
    install_dir: gimpplugindir / 'environ',
  )
endif

if platform_windows and not meson.is_cross_build() and python.found()
  install_data(
    'pygimp.env',
    install_dir: gimpplugindir / 'environ',
  )
endif