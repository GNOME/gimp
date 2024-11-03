if platform_osx
  add_project_arguments('-ObjC', language : 'c')
endif

gimpversion = configure_file(
  input : 'gimpversion.h.in',
  output: 'gimpversion.h',
  configuration: versionconfig,
)

stamp_base_enums = custom_target('stamp-gimpbaseenums.h',
  input : [
    files(
      'gimpbaseenums.h'
    ),
  ],
  output: [ 'stamp-gimpbaseenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'gimpbase',
    '#include <glib-object.h>\n'       +
    '#include "gimpbasetypes.h"\n'     +
    '#include "libgimp/libgimp-intl.h"\n',
    '',
    libgimp_mkenums_dtails
  ],
  build_by_default: true
)

stamp_compat_enums = custom_target('stamp-gimpcompatenums.h',
  input : [
    files(
      'gimpcompatenums.h'
    ),
  ],
  output: [ 'stamp-gimpcompatenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(), meson.current_build_dir(),
    'gimpcompat',
    '#include <glib-object.h>\n'       +
    '#include "gimpbasetypes.h"\n',
    '#include "libgimp/libgimp-intl.h"',
  ],
  build_by_default: true
)

libgimpbase_sources_introspectable = files(
  'gimpbasetypes.c',
  'gimpchecks.c',
  'gimpchoice.c',
  'gimpcpuaccel.c',
  'gimpenv.c',
  'gimpexportoptions.c',
  'gimpmemsize.c',
  'gimpmetadata.c',
  'gimpparamspecs.c',
  'gimpparasite.c',
  'gimpparasiteio.c',
  'gimprectangle.c',
  'gimpunit.c',
  'gimputils.c',
  'gimpvaluearray.c',
)

libgimpbase_sources = [
  libgimpbase_sources_introspectable,
  'gimpbase-private.c',
  'gimpprotocol.c',
  'gimpreloc.c',
  'gimpsignal.c',
  'gimpwire.c',

  'gimpbaseenums.c',
  stamp_base_enums,

  'gimpcompatenums.c',
  stamp_compat_enums
]

libgimpbase_headers_introspectable = files(
  'gimpbaseenums.h',
  'gimpbasetypes.h',
  'gimpchecks.h',
  'gimpchoice.h',
  'gimpcpuaccel.h',
  'gimpenv.h',
  'gimpexportoptions.h',
  'gimplimits.h',
  'gimpmemsize.h',
  'gimpmetadata.h',
  'gimpparamspecs.h',
  'gimpparasite.h',
  'gimpparasiteio.h',
  'gimprectangle.h',
  'gimpunit.h',
  'gimputils.h',
  'gimpvaluearray.h',
) + [
  gimpversion,
]

libgimpbase_headers = [
  libgimpbase_headers_introspectable,
  'gimpbase.h',
  gimpversion,
]

libgimpbase_introspectable = [
  libgimpbase_sources_introspectable,
  libgimpbase_headers_introspectable,
]

libgimpbase = library('gimpbase-' + gimp_api_version,
  libgimpbase_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gio, math,
    # optionally depend on libexecinfo on platforms where it is not
    # internal to the libc.
    opt_execinfo,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="LibGimpBase"',
    '-DGIMP_BASE_COMPILATION',
  ],
  vs_module_defs: 'gimpbase.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpbase_headers,
  subdir: gimp_api_name / 'libgimpbase',
)

# Test program, not installed
executable('test-cpu-accel',
  'test-cpu-accel.c',
  include_directories: rootInclude,
  dependencies: [
    glib,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="LibGimpBase"',
    '-DGIMP_BASE_COMPILATION',
  ],
  link_with: [
    libgimpbase,
  ],
  install: false,
)
