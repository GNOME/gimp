libappfile_sources = [
  'file-import.c',
  'file-open.c',
  'file-remote.c',
  'file-save.c',
  'file-utils.c',
]

libappfile = static_library('appfile',
  libappfile_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-File"',
  dependencies: [
    gegl, gdk_pixbuf, gexiv2
  ],
)
