stamp_plug_in_enums = custom_target('stamp-plug-in-enums.h',
  input : [
    files(
      'plug-in-enums.h'
    ),
  ],
  output: [ 'stamp-plug-in-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'plug-in-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libappplugin_sources = [
  'ligmaenvirontable.c',
  'ligmagpparams.c',
  'ligmainterpreterdb.c',
  'ligmaplugin-cleanup.c',
  'ligmaplugin-context.c',
  'ligmaplugin-message.c',
  'ligmaplugin-proc.c',
  'ligmaplugin-progress.c',
  'ligmaplugin.c',
  'ligmaplugindebug.c',
  'ligmaplugindef.c',
  'ligmapluginerror.c',
  'ligmapluginmanager-call.c',
  'ligmapluginmanager-data.c',
  'ligmapluginmanager-file.c',
  'ligmapluginmanager-help-domain.c',
  'ligmapluginmanager-menu-branch.c',
  'ligmapluginmanager-query.c',
  'ligmapluginmanager-restore.c',
  'ligmapluginmanager.c',
  'ligmapluginprocedure.c',
  'ligmapluginprocframe.c',
  'ligmapluginshm.c',
  'ligmatemporaryprocedure.c',
  'plug-in-menu-path.c',
  'plug-in-rc.c',

  'plug-in-enums.c',
  stamp_plug_in_enums,

  appcoremarshal[1],
]

libappplugin = static_library('appplug-in',
  libappplugin_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Plug-In"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
