
libligmamodule_sources = files(
  'ligmamodule.c',
  'ligmamoduledb.c',
)

libligmamodule_headers = files(
  'ligmamodule.h',
  'ligmamoduledb.h',
  'ligmamoduletypes.h',
)

libligmamodule_introspectable = [
  libligmamodule_sources,
  libligmamodule_headers,
]

libligmamodule = library('ligmamodule-' + ligma_api_version,
  libligmamodule_sources,
  include_directories: rootInclude,
  dependencies: [
    gio, glib, gmodule,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaModule"', '-DLIGMA_MODULE_COMPILATION', ],
  link_with: [
    libligmabase,
    libligmaconfig,
  ],
  vs_module_defs: 'ligmamodule.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmamodule_headers,
  subdir: ligma_api_name / 'libligmamodule',
)
