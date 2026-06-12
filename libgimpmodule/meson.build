
libgimpmodule_sources = files(
  'gimpmodule.c',
  'gimpmoduledb.c',
)

libgimpmodule_headers = files(
  'gimpmodule.h',
  'gimpmoduledb.h',
  'gimpmoduletypes.h',
)

libgimpmodule_introspectable = [
  libgimpmodule_sources,
  libgimpmodule_headers,
]

#Needed otherwise defcheck.py on libgimp/meson.build fails
libgimpmodule_def = configure_file(
  input: 'gimpmodule.def',
  output: 'gimpmodule.def',
  copy: true
)

libgimpmodule = library('gimpmodule-' + gimp_api_version,
  libgimpmodule_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gio, glib, gmodule,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpModule"', '-DGIMP_MODULE_COMPILATION', ],
  link_with: [
    libgimpbase,
    libgimpconfig,
  ],
  vs_module_defs: libgimpmodule_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libgimpmodule_headers,
  subdir: gimp_api_name / 'libgimpmodule',
)
