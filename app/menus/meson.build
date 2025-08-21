libappmenus_sources = [
  'dockable-menu.c',
  'file-menu.c',
  'filters-menu.c',
  'image-menu.c',
  'menus.c',
  'plug-in-menus.c',
  'shortcuts-rc.c',
  'tool-options-menu.c',
  'window-menu.c',
  'windows-menu.c',
]

libappmenus = static_library('appmenus',
  libappmenus_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Menus"',
  dependencies: [
    gegl, gtk3, gexiv2
  ],
)
