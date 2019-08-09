
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

libgimpbase_sources_introspectable = [
  'gimpbase-private.c',
  'gimpbasetypes.c',
  'gimpchecks.c',
  'gimpcpuaccel.c',
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
]

libgimpbase_sources = [
  libgimpbase_sources_introspectable,
  gimpbaseenums,
  gimpcompatenums,
  gimpversion,
]

libgimpbase_headers_introspectable = [
  'gimpbase-private.h',
  'gimpbase.h',
  'gimpbaseenums.h',
  'gimpbasetypes.h',
  'gimpchecks.h',
  'gimpcompatenums.h',
  'gimpcpuaccel.h',
  'gimpenv.h',
  'gimplimits.h',
  'gimpmemsize.h',
  'gimpmetadata.h',
  'gimpparam.h',
  'gimpparasite.h',
  'gimpparasiteio.h',
  'gimpprotocol.h',
  'gimprectangle.h',
  'gimpreloc.h',
  'gimpsignal.h',
  'gimpunit.h',
  'gimputils.h',
  'gimpvaluearray.h',
  'gimpversion.h',
  'gimpwire.h',
]

libgimpbase_headers_public = [
  'gimpbase.h',
  'gimpbaseenums.h',
  'gimpbasetypes.h',
  'gimpchecks.h',
  'gimpcpuaccel.h',
  'gimpenv.h',
  'gimplimits.h',
  'gimpmemsize.h',
  'gimpmetadata.h',
  'gimpparam.h',
  'gimpparamspecs.h',
  'gimpparasite.h',
  'gimpparasiteio.h',
  'gimprectangle.h',
  'gimpsignal.h',
  'gimpunit.h',
  'gimputils.h',
  'gimpvaluearray.h',
  gimpversion,
]


libgimpbase = library('gimpbase-' + gimp_api_version,
  libgimpbase_sources,
  include_directories: rootInclude,
  dependencies: [
    gexiv2, gio, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpBase"', '-DGIMP_BASE_COMPILATION', ],
  vs_module_defs: 'gimpbase.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpbase_headers_public,
  subdir: join_paths(gimp_api_name, 'libgimpbase'),
)

# libgimpbase_gir = gnome.generate_gir(libgimpbase,
#   sources: [
#     libgimpbase_sources_introspectable,
#     libgimpbase_headers_introspectable,
#   ],
#   nsversion: api_version,
#   namespace: 'Gimp',
#   identifier_prefix: 'Gimp',
#   includes: [
#     'GLib-2.0',
#     'GObject-2.0',
#   ],
#   include_directories: rootInclude,
#   install: true,
# )

# Test program, not installed
executable('test-cpu-accel',
  'test-cpu-accel.c',
  include_directories: rootInclude,
  dependencies: [
    glib,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpBase"', '-DGIMP_BASE_COMPILATION', ],
  link_with: [
    libgimpbase,
  ],
  install: false,
)
