# Unlike other enums file, we don't use the mkenums_wrap because this
# one is not versionned in the repository (not sure why). Moreover the
# options are quite different from the other generated enums, so it
# didn't make sense to overdo it.
gimpthumbenums = custom_target('gimpthumb-enums.c',
  input : [ 'gimpthumb-enums.h', ],
  output: [ 'gimpthumb-enums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'+
              '#include <glib-object.h>\n'+
              '#include "gimpthumb-enums.h"\n',
    '--fprod','/* enumerations from "@filename@" */',
    '--vhead','GType\n'+
              '@enum_name@_get_type (void)\n'+
              '{\n'+
              '  static const G@Type@Value values[] =\n'+
              '  {',
    '--vprod','    { @VALUENAME@, @valuedesc@, "@valuenick@" },',
    '--vtail','    { 0, NULL, NULL }\n'+
              '  };\n'+
              '\n'+
              '  static GType type = 0;\n'+
              '\n'+
              '  if (G_UNLIKELY (! type))\n'+
              '    type = g_@type@_register_static ("@EnumName@", values);\n'+
              '\n'+
              '  return type;\n'+
              '}\n',
    '@INPUT@',
  ],
  capture: true,
)

libgimpthumb_sources_introspectable = files(
  'gimpthumb-error.c',
  'gimpthumb-utils.c',
  'gimpthumbnail.c',
)

libgimpthumb_sources = [
  libgimpthumb_sources_introspectable,
  gimpthumbenums,
]

libgimpthumb_headers_introspectable = files(
  'gimpthumb-enums.h',
  'gimpthumb-error.h',
  'gimpthumb-types.h',
  'gimpthumb-utils.h',
  'gimpthumbnail.h',
)

libgimpthumb_headers = [
  libgimpthumb_headers_introspectable,
  'gimpthumb.h',
]

libgimpthumb_introspectable = [
  libgimpthumb_sources_introspectable,
  libgimpthumb_headers_introspectable,
]

libgimpthumb = library('gimpthumb-'+ gimp_api_version,
  libgimpthumb_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, glib, gobject, gdk_pixbuf, gio,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpThumb"', '-DGIMP_THUMB_COMPILATION', ],
  link_with:  [
    libgimpbase,
  ],
  vs_module_defs: 'gimpthumb.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpthumb_headers,
  subdir: gimp_api_name / 'libgimpthumb',
)


# Test program, not installed
gimp_thumbnail_list = executable('gimp-thumbnail-list',
  'gimp-thumbnail-list.c',
  include_directories: rootInclude,
  dependencies: [
    gdk_pixbuf,
  ],
  c_args: '-DG_LOG_DOMAIN="LibGimpThumb"',
  link_with: [
    libgimpbase,
    libgimpthumb,
  ],
  install: false,
)
