
gimpthumbenums = custom_target('gimpthumb-enums.c',
  input : [ 'gimpthumb-enums.h', ],
  output: [ 'gimpthumb-enums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'+
              '#include <glib-object.h>\n'+
              '#include "gimpthumb-enums.h"\n',
    '--fprod','/* enumerations from "@filename@" */\n',
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

libgimpthumb_sources = [
  'gimpthumb-error.c',
  'gimpthumb-utils.c',
  'gimpthumbnail.c',
  gimpthumbenums,
]

libgimpthumb_headers = [
  'gimpthumb-enums.h',
  'gimpthumb-error.h',
  'gimpthumb-types.h',
  'gimpthumb-utils.h',
  'gimpthumb.h',
  'gimpthumbnail.h',
]


libgimpthumb_cflags = [
  '-DG_LOG_DOMAIN="LibGimpThumb"',
  '-DGIMP_THUMB_COMPILATION',
]

libgimpthumb = library('gimpthumb-'+ api_version,
  libgimpthumb_sources,
  include_directories: rootInclude,
  dependencies: [
    gdk_pixbuf, glib, gio, gobject,
  ],
  c_args:     [ libgimpthumb_cflags, ],
  link_with:  [
    libgimpbase,
  ],
  vs_module_defs: 'gimpthumb.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpthumb_headers,
  subdir: join_paths(gimp_api_name, 'libgimpthumb'),
)


# Test program, not installed
gimp_thumbnail_list = executable('gimp-thumbnail-list',
  'gimp-thumbnail-list.c',
  include_directories: rootInclude,
  dependencies: [
    gdk_pixbuf,
  ],
  c_args:     [ libgimpthumb_cflags, ],
  link_with:  [
    libgimpbase,
    libgimpthumb,
  ],
  install: false,
)
