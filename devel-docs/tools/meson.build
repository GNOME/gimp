if have_doc_shooter

doc_shooter_sources = [
  'shadow.c',
  'shooter.c',
  'units.c',
  'widgets.c',
]

doc_shooter = executable('doc-shooter',
  doc_shooter_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gdk_pixbuf, gegl, fontconfig, math, x11, xext,
  ],
  c_args: '-DTOP_SRCDIR="' + meson.source_root() + '"',
  link_with: [
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpmodule,
    libgimpwidgets,
  ],
  install: false,
)

endif
