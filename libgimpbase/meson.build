
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
    gimp_mkenums_custom_target_commonargs,
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
            + '#include "@INPUT@"\n'
            + '#include "libgimp/libgimp-intl.h"\n',
    gimp_mkenums_custom_target_commonargs,
  ],
  capture: true,
)

libgimpbase_sources = [
  'gimpbase-private.c',
  'gimpbasetypes.c',
  'gimpchecks.c',
  'gimpcpuaccel.c',
  'gimpdatafiles.c',
  'gimpenv.c',
  'gimpmemsize.c',
  'gimpmetadata.c',
  'gimpparasite.c',
  'gimpparasiteio.c',
  'gimpprotocol.c',
  'gimprectangle.c',
  'gimpreloc.c',
  'gimpsignal.c',
  'gimpunit.c',
  'gimputils.c',
  'gimpvaluearray.c',
  'gimpwire.c',
  gimpbaseenums,
  gimpcompatenums,
  gimpversion,
]

libgimpbase_headers = [
  'gimpbase.h',
  'gimpbaseenums.h',
  'gimpbasetypes.h',
  'gimpcpuaccel.h',
  'gimplimits.h',
  'gimpparam.h',
  'gimpchecks.h',
  'gimpdatafiles.h',
  'gimpenv.h',
  'gimpmemsize.h',
  'gimpmetadata.h',
  'gimpparasite.h',
  'gimpparasiteio.h',
  'gimprectangle.h',
  'gimpsignal.h',
  'gimpunit.h',
  'gimputils.h',
  'gimpvaluearray.h',
  gimpversion,
]

libgimpbase_cflags = [
  '-DG_LOG_DOMAIN="LibGimpBase"',
  '-DGIMP_BASE_COMPILATION',
]

libgimpbase = library('gimpbase-' + api_version,
  libgimpbase_sources,
  include_directories: rootInclude,
  dependencies: [
    gexiv2, gio, math,
  ],
  c_args:     [ libgimpbase_cflags, ],
  vs_module_defs: 'gimpbase.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpbase_headers,
  subdir: join_paths(gimp_api_name, 'libgimpbase'),
)


# Test program, not installed
executable('test-cpu-accel',
  'test-cpu-accel.c',
  include_directories: rootInclude,
  dependencies: [
    glib,
  ],
  c_args:     [ libgimpbase_cflags, ],
  link_with:  [
    libgimpbase,
  ],
  install: false,
)
