if platform_osx
  add_project_arguments('-ObjC', language : 'c')
endif

ligmaversion = configure_file(
  input : 'ligmaversion.h.in',
  output: 'ligmaversion.h',
  configuration: versionconfig,
)

stamp_base_enums = custom_target('stamp-ligmabaseenums.h',
  input : [
    files(
      'ligmabaseenums.h'
    ),
  ],
  output: [ 'stamp-ligmabaseenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'ligmabase',
    '#include <glib-object.h>\n'       +
    '#undef LIGMA_DISABLE_DEPRECATED\n' +
    '#include "ligmabasetypes.h"\n'     +
    '#include "libligma/libligma-intl.h"\n',
    '',
    libligma_mkenums_dtails
  ],
  build_by_default: true
)

stamp_compat_enums = custom_target('stamp-ligmacompatenums.h',
  input : [
    files(
      'ligmacompatenums.h'
    ),
  ],
  output: [ 'stamp-ligmacompatenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(), meson.current_build_dir(),
    'ligmacompat',
    '#include <glib-object.h>\n'       +
    '#include "ligmabasetypes.h"\n',
    '#include "libligma/libligma-intl.h"',
  ],
  build_by_default: true
)

libligmabase_sources_introspectable = files(
  'ligmabasetypes.c',
  'ligmachecks.c',
  'ligmacpuaccel.c',
  'ligmaenv.c',
  'ligmamemsize.c',
  'ligmametadata.c',
  'ligmaparamspecs.c',
  'ligmaparasite.c',
  'ligmaparasiteio.c',
  'ligmarectangle.c',
  'ligmasignal.c',
  'ligmaunit.c',
  'ligmautils.c',
  'ligmavaluearray.c',
)

libligmabase_sources = [
  libligmabase_sources_introspectable,
  'ligmabase-private.c',
  'ligmaprotocol.c',
  'ligmareloc.c',
  'ligmawire.c',

  'ligmabaseenums.c',
  stamp_base_enums,

  'ligmacompatenums.c',
  stamp_compat_enums
]

libligmabase_headers_introspectable = files(
  'ligmabaseenums.h',
  'ligmabasetypes.h',
  'ligmachecks.h',
  'ligmacpuaccel.h',
  'ligmaenv.h',
  'ligmalimits.h',
  'ligmamemsize.h',
  'ligmametadata.h',
  'ligmaparamspecs.h',
  'ligmaparasite.h',
  'ligmaparasiteio.h',
  'ligmarectangle.h',
  'ligmasignal.h',
  'ligmaunit.h',
  'ligmautils.h',
  'ligmavaluearray.h',
) + [
  ligmaversion,
]

libligmabase_headers = [
  libligmabase_headers_introspectable,
  'ligmabase.h',
  ligmaversion,
]

libligmabase_introspectable = [
  libligmabase_sources_introspectable,
  libligmabase_headers_introspectable,
]

libligmabase = library('ligmabase-' + ligma_api_version,
  libligmabase_sources,
  include_directories: rootInclude,
  dependencies: [
    gexiv2, gio, math,
    # optionally depend on libexecinfo on platforms where it is not
    # internal to the libc.
    opt_execinfo,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="LibLigmaBase"',
    '-DLIGMA_BASE_COMPILATION',
  ],
  vs_module_defs: 'ligmabase.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmabase_headers,
  subdir: ligma_api_name / 'libligmabase',
)

# Test program, not installed
executable('test-cpu-accel',
  'test-cpu-accel.c',
  include_directories: rootInclude,
  dependencies: [
    glib,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="LibLigmaBase"',
    '-DLIGMA_BASE_COMPILATION',
  ],
  link_with: [
    libligmabase,
  ],
  install: false,
)
