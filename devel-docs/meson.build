devel_docs_build_root = meson.current_build_dir()

version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)

scan_args_common = [
  '--deprecated-guards=GIMP_DISABLE_DEPRECATED',
]

mkdb_args_common = [
  '--name-space=gimp',
]


if get_option('gi-docgen')
  subdir('reference')
endif

if get_option('g-ir-doc')
  subdir('g-ir-docs')
endif
