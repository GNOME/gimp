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
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "gimp-intl.h"'
  ],
  build_by_default: true
)

libappplugin_sources = [
  'gimpenvirontable.c',
  'gimpgpparams.c',
  'gimpinterpreterdb.c',
  'gimpplugin-cleanup.c',
  'gimpplugin-context.c',
  'gimpplugin-message.c',
  'gimpplugin-proc.c',
  'gimpplugin-progress.c',
  'gimpplugin.c',
  'gimpplugindebug.c',
  'gimpplugindef.c',
  'gimppluginerror.c',
  'gimppluginmanager-call.c',
  'gimppluginmanager-data.c',
  'gimppluginmanager-file.c',
  'gimppluginmanager-help-domain.c',
  'gimppluginmanager-menu-branch.c',
  'gimppluginmanager-query.c',
  'gimppluginmanager-restore.c',
  'gimppluginmanager.c',
  'gimppluginprocedure.c',
  'gimppluginprocframe.c',
  'gimppluginshm.c',
  'gimptemporaryprocedure.c',
  'plug-in-menu-path.c',
  'plug-in-rc.c',

  'plug-in-enums.c',
  stamp_plug_in_enums,

  appcoremarshal[1],
]

libappplugin = static_library('appplug-in',
  libappplugin_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Plug-In"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2,
  ],
)
