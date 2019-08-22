version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)
if get_option('gtk-doc-app')
  subdir('app')
endif

subdir('libgimp')
subdir('libgimpbase')
subdir('libgimpcolor')
subdir('libgimpconfig')
subdir('libgimpmath')
subdir('libgimpmodule')
subdir('libgimpthumb')
subdir('libgimpwidgets')
subdir('tools')
