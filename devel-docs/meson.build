if gtkdoc_scan.found()
version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)
  subdir('app')
  subdir('libgimp')
  subdir('libgimpbase')
  subdir('libgimpcolor')
  subdir('libgimpconfig')
  subdir('libgimpmath')
  subdir('libgimpmodule')
  subdir('libgimpthumb')
  subdir('libgimpwidgets')
  subdir('tools')
endif
