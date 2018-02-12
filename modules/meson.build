
modules_deps = [
  gtk3, babl, gegl, math,
]

color_selector_libs = [
  libgimpcolor,
  libgimpconfig,
  libgimpmodule,
  libgimpwidgets,
]
controller_libs = [
  libgimpmodule,
  libgimpwidgets,
]
display_filter_libs = [
  libgimpbase,
  libgimpcolor,
  libgimpconfig,
  libgimpmodule,
  libgimpwidgets,
]

# Name, Sources, deps, link.
modules = [
  [ 'color-selector-cmyk',
    [ 'color-selector-cmyk.c', ],
    [], [ color_selector_libs, ],
  ],
  [ 'color-selector-water',
    [ 'color-selector-water.c', ],
    [ ], [ color_selector_libs, ],
  ],
  [ 'color-selector-wheel',
    [ 'color-selector-wheel.c', 'gimpcolorwheel.c', ],
    [ ], [ color_selector_libs, ],
  ],
  [ 'display-filter-clip-warning',
    [ 'display-filter-clip-warning.c', ],
    [], [ display_filter_libs, ],
  ],
  [ 'display-filter-color-blind',
    [ 'display-filter-color-blind.c', ],
    [], [ display_filter_libs, ],
  ],
  [ 'display-filter-gamma',
    [ 'display-filter-gamma.c', ],
    [ ], [ display_filter_libs, ],
  ],
  [ 'display-filter-high-contrast',
    [ 'display-filter-high-contrast.c', ],
    [ ], [ display_filter_libs, ],
  ],
]

if have_linuxinput
  modules += [
    [ 'controller-linux-input',
      [ 'controller-linux-input.c', 'gimpinputdevicestore-gudev.c', ],
      [ gudev, ], [ controller_libs, ],
    ],
  ]
endif

if directx.found()
  modules += [
    [ 'controller-dx-input',
      [ 'controller-dx-dinput.c', 'gimpinputdevicestore-dx.c', ],
      [ directx,  ], [ controller_libs, '-lrpcrt4', ],
    ],
  ]
endif

if not platform_win32
  modules += [
    [ 'controller-midi',
      [ 'controller-midi.c', ],
      [ alsa, ], [ controller_libs, ],
    ],
  ]
endif


foreach module : modules
  name = module[0]
  srcs = module[1]
  deps = module[2]
  link = module[3]

  library(name,
    srcs,
    include_directories: rootInclude,
    dependencies: modules_deps + deps,
    link_with: link,
    install: true,
    install_dir: join_paths(gimpplugindir, 'modules'),
  )
endforeach
