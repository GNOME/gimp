# Unlike other enums file, we don't use the mkenums_wrap because this
# one is not versionned in the repository (not sure why). Moreover the
# options are quite different from the other generated enums, so it
# didn't make sense to overdo it.
ligmathumbenums = custom_target('ligmathumb-enums.c',
  input : [ 'ligmathumb-enums.h', ],
  output: [ 'ligmathumb-enums.c', ],
  command: [
    ligma_mkenums,
    '--fhead','#include "config.h"\n'+
              '#include <glib-object.h>\n'+
              '#include "ligmathumb-enums.h"\n',
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

libligmathumb_sources_introspectable = files(
  'ligmathumb-error.c',
  'ligmathumb-utils.c',
  'ligmathumbnail.c',
)

libligmathumb_sources = [
  libligmathumb_sources_introspectable,
  ligmathumbenums,
]

libligmathumb_headers_introspectable = files(
  'ligmathumb-enums.h',
  'ligmathumb-error.h',
  'ligmathumb-types.h',
  'ligmathumb-utils.h',
  'ligmathumbnail.h',
)

libligmathumb_headers = [
  libligmathumb_headers_introspectable,
  'ligmathumb.h',
]

libligmathumb_introspectable = [
  libligmathumb_sources_introspectable,
  libligmathumb_headers_introspectable,
]

libligmathumb = library('ligmathumb-'+ ligma_api_version,
  libligmathumb_sources,
  include_directories: rootInclude,
  dependencies: [
    glib, gobject, gdk_pixbuf, gio,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaThumb"', '-DLIGMA_THUMB_COMPILATION', ],
  link_with:  [
    libligmabase,
  ],
  vs_module_defs: 'ligmathumb.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmathumb_headers,
  subdir: ligma_api_name / 'libligmathumb',
)


# Test program, not installed
ligma_thumbnail_list = executable('ligma-thumbnail-list',
  'ligma-thumbnail-list.c',
  include_directories: rootInclude,
  dependencies: [
    gdk_pixbuf,
  ],
  c_args: '-DG_LOG_DOMAIN="LibLigmaThumb"',
  link_with: [
    libligmabase,
    libligmathumb,
  ],
  install: false,
)
