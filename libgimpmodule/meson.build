
libgimpmodule_sources = [
  'gimpmodule.c',
  'gimpmoduledb.c',
]

libgimpmodule_headers = [
  'gimpmodule.h',
  'gimpmoduledb.h',
  'gimpmoduletypes.h',
]


libgimpmodule = library('gimpmodule-' + api_version,
  libgimpmodule_sources,
  include_directories: rootInclude,
  dependencies: [
    gio, glib, gmodule,
  ],
  c_args:     [
    '-DG_LOG_DOMAIN="LibGimpModule"',
    '-DGIMP_MODULE_COMPILATION',
  ],
  link_with:  [
    libgimpbase,
    libgimpconfig,
  ],
  vs_module_defs: 'gimpmodule.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpmodule_headers,
  subdir: join_paths(gimp_api_name, 'libgimpmodule'),
)
