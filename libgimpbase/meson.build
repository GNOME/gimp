if platform_osx
  add_project_arguments('-ObjC', language : 'c')
endif

gimpversion = configure_file(
  input : 'gimpversion.h.in',
  output: 'gimpversion.h',
  configuration: versionconfig,
)


gimpbaseenums = custom_target('gimpbaseenums.c',
  input : [ 'gimpbaseenums.h', ],
  output: [ 'gimpbaseenums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <glib-object.h>\n'
            + '#undef GIMP_DISABLE_DEPRECATED\n'
            + '#include "gimpbasetypes.h"\n'
            + '#include "libgimp/libgimp-intl.h"\n',
    libgimp_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
)

gimpcompatenums = custom_target('gimpcompatenums.c',
  input : [ 'gimpcompatenums.h', ],
  output: [ 'gimpcompatenums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <glib-object.h>\n'
            + '#include "gimpbasetypes.h"\n'
            + '#include "@basename@"\n'
            + '#include "libgimp/libgimp-intl.h"\n',
    libgimp_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
)

libgimpbase_sources_introspectable = files(
  'gimpbasetypes.c',
  'gimpchecks.c',
  'gimpcpuaccel.c',
  'gimpenv.c',
  'gimpmemsize.c',
  'gimpmetadata.c',
  'gimpparamspecs.c',
  'gimpparasite.c',
  'gimpparasiteio.c',
  'gimprectangle.c',
  'gimpsignal.c',
  'gimpunit.c',
  'gimputils.c',
  'gimpvaluearray.c',
)

libgimpbase_sources = [
  libgimpbase_sources_introspectable,
  'gimpbase-private.c',
  'gimpprotocol.c',
  'gimpreloc.c',
  'gimpwire.c',
  gimpbaseenums,
  gimpcompatenums,
]

libgimpbase_headers_introspectable = files(
  'gimpbaseenums.h',
  'gimpbasetypes.h',
  'gimpchecks.h',
  'gimpcpuaccel.h',
  'gimpenv.h',
  'gimplimits.h',
  'gimpmemsize.h',
  'gimpmetadata.h',
  'gimpparamspecs.h',
  'gimpparasite.h',
  'gimpparasiteio.h',
  'gimprectangle.h',
  'gimpsignal.h',
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
    gexiv2, gio, math,
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
