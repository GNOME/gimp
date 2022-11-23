
ligmadbusservice_gen = gnome.gdbus_codegen(
  'ligmadbusservice-generated',
  'dbus-service.xml',
  interface_prefix: 'org.ligma.LIGMA.',
  namespace: 'LigmaDBusService',
)

libappgui_sources = [
  'ligmaapp.c',
  'ligmadbusservice.c',
  'ligmauiconfigurer.c',
  'gui-message.c',
  'gui-unique.c',
  'gui-vtable.c',
  'gui.c',
  'icon-themes.c',
  'modifiers.c',
  'session.c',
  'splash.c',
  'themes.c',
  ligmadbusservice_gen,
]

# Workaround for generated header included in other directories.
configInclude = include_directories('.')


libappgui = static_library('appgui',
  libappgui_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-GUI"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gio_specific, gtk3, gtk3_macos
  ],
  install: false,
)
