plugin_name = 'file-psd'

plugin_sources = [
  'psd-image-res-load.c',
  'psd-layer-res-load.c',
  'psd-load.c',
  'psd-save.c',
  'psd-thumb-load.c',
  'psd-util.c',
  'psd.c',
]

if platform_win32
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

executable(plugin_name,
  plugin_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, libjpeg, math, zlib,
  ],
  link_with: [
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpui,
    libgimpwidgets,
  ],
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)
