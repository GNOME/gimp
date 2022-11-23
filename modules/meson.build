
modules_deps = [
  gtk3, babl, gegl, math,
]

color_selector_libs = [
  libligmacolor,
  libligmaconfig,
  libligmamodule,
  libligmawidgets,
]
controller_libs = [
  libligmamodule,
  libligmawidgets,
]
display_filter_libs = [
  libligmabase,
  libligmacolor,
  libligmaconfig,
  libligmamodule,
  libligmawidgets,
]

# Name, Sources, deps, link.
modules = [
  {
    'name': 'color-selector-cmyk',
    'link': color_selector_libs,
  }, {
    'name': 'color-selector-water',
    'link': color_selector_libs,
  }, {
    'name': 'color-selector-wheel',
    'srcs': [ 'color-selector-wheel.c', 'ligmacolorwheel.c', ],
    'link': color_selector_libs,
  }, {
    'name': 'display-filter-clip-warning',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-color-blind',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-aces-rrt',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-gamma',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-high-contrast',
    'link': display_filter_libs,
  },
]

if have_linuxinput
  modules += {
    'name': 'controller-linux-input',
    'srcs': [ 'controller-linux-input.c', 'ligmainputdevicestore-gudev.c', ],
    'deps': gudev,
    'link': controller_libs,
  }
endif

if directx.found()
  modules += {
    'name': 'controller-dx-dinput',
    'srcs': [ 'controller-dx-dinput.c', 'ligmainputdevicestore-dx.c', ],
    'deps': directx,
    'link': [ controller_libs, ],
    'link-args': [ '-lrpcrt4', ],
  }
endif

if not platform_windows
  modules += {
    'name': 'controller-midi',
    'deps': alsa,
    'link': controller_libs,
  }
endif


foreach module : modules
  name = module.get('name')
  srcs = module.get('srcs', name + '.c')
  deps = module.get('deps', [])
  link = module.get('link', [])
  link_args = module.get('link-args', [])

  library(name,
    srcs,
    include_directories: rootInclude,
    dependencies: modules_deps + [ deps ],
    link_with: link,
    link_args: link_args,
    install: true,
    install_dir: ligmaplugindir / 'modules',
  )
endforeach
