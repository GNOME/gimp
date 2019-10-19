
apppluginenums = custom_target('plug-in-enums.c',
  input : [ 'plug-in-enums.h', ],
  output: [ 'plug-in-enums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "@basename@"\n'
            + '#include "gimp-intl.h"\n',
    app_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
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
  'gimppluginmanager-locale-domain.c',
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
  apppluginenums,

  appcoremarshal,
]

libappplugin = static_library('appplug-in',
  libappplugin_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Plug-In"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
