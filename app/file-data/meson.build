
libappfiledata_sources = [
  'file-data-gbr.c',
  'file-data-gex.c',
  'file-data-gih.c',
  'file-data-pat.c',
  'file-data.c',
]

libappfiledata = static_library('appfiledata',
  libappfiledata_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-File-Data"',
  dependencies: [
    appstream,
    cairo,
    gdk_pixbuf,
    gegl,
    gexiv2,
    libarchive,
  ],
)
