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
    gtk3, gdk_pixbuf, gegl, fontconfig, math,
  ],
  c_args: '-DTOP_SRCDIR="@0@"'.format(meson.source_root()),
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
