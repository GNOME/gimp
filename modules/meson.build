
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
  {
    'name': 'color-selector-cmyk',
    'link': color_selector_libs,
  }, {
    'name': 'color-selector-water',
    'link': color_selector_libs,
  }, {
    'name': 'color-selector-wheel',
    'srcs': [ 'color-selector-wheel.c', 'gimpcolorwheel.c', ],
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
    'srcs': [ 'controller-linux-input.c', 'gimpinputdevicestore-gudev.c', ],
    'deps': gudev,
    'link': controller_libs,
  }
endif

if directx.found()
  modules += {
    'name': 'controller-dx-dinput',
    'srcs': [ 'controller-dx-dinput.c', 'gimpinputdevicestore-dx.c', ],
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
    install_dir: gimpplugindir / 'modules',
  )
endforeach
