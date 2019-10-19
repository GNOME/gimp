
gimpwidgetsenums = custom_target('gimpwidgetsenums.c',
  input : [ 'gimpwidgetsenums.h', ],
  output: [ 'gimpwidgetsenums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "@basename@"\n'
            + '#include "libgimp/libgimp-intl.h"\n',
    libgimp_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
)

gimpwidgetsmarshal = gnome.genmarshal('gimpwidgetsmarshal',
  prefix: '_gimp_widgets_marshal',
  sources: 'gimpwidgetsmarshal.list',
  install_header: false,
)

libgimpwidgets_sources_introspectable = files(
  'gimpbrowser.c',
  'gimpbusybox.c',
  'gimpbutton.c',
  'gimpcairo-utils.c',
  'gimpcellrenderercolor.c',
  'gimpcellrenderertoggle.c',
  'gimpchainbutton.c',
  'gimpcolorarea.c',
  'gimpcolorbutton.c',
  'gimpcolordisplay.c',
  'gimpcolordisplaystack.c',
  'gimpcolorhexentry.c',
  'gimpcolornotebook.c',
  'gimpcolorprofilechooserdialog.c',
  'gimpcolorprofilecombobox.c',
  'gimpcolorprofilestore.c',
  'gimpcolorprofileview.c',
  'gimpcolorselection.c',
  'gimpcolorselector.c',
  'gimpdialog.c',
  'gimpenumcombobox.c',
  'gimpenumlabel.c',
  'gimpenumstore.c',
  'gimpenumwidgets.c',
  'gimpfileentry.c',
  'gimpframe.c',
  'gimphelpui.c',
  'gimphintbox.c',
  'gimpicons.c',
  'gimpintcombobox.c',
  'gimpintstore.c',
  'gimpmemsizeentry.c',
  'gimpnumberpairentry.c',
  'gimpoffsetarea.c',
  'gimppageselector.c',
  'gimppatheditor.c',
  'gimppickbutton.c',
  'gimppreview.c',
  'gimppreviewarea.c',
  'gimppropwidgets.c',
  'gimpquerybox.c',
  'gimpruler.c',
  'gimpscaleentry.c',
  'gimpscrolledpreview.c',
  'gimpsizeentry.c',
  'gimpspinbutton.c',
  'gimpstringcombobox.c',
  'gimpunitcombobox.c',
  'gimpunitstore.c',
  'gimpwidgets-error.c',
  'gimpwidgets.c',
  'gimpwidgetsutils.c',
  'gimpzoommodel.c',
)

libgimpwidgets_sources = [
  libgimpwidgets_sources_introspectable,
  'gimpcolorscale.c',
  'gimpcolorscales.c',
  'gimpcolorselect.c',
  'gimpcontroller.c',
  'gimpeevl.c',
  'gimpwidgets-private.c',

  gimpwidgetsenums,
  gimpwidgetsmarshal,
  icons_imgs_sources,
  cursors_sources,
  pickers_sources,
]

libgimpwidgets_headers_introspectable = files(
  'gimpbrowser.h',
  'gimpbusybox.h',
  'gimpbutton.h',
  'gimpcairo-utils.h',
  'gimpcellrenderercolor.h',
  'gimpcellrenderertoggle.h',
  'gimpchainbutton.h',
  'gimpcolorarea.h',
  'gimpcolorbutton.h',
  'gimpcolordisplay.h',
  'gimpcolordisplaystack.h',
  'gimpcolorhexentry.h',
  'gimpcolornotebook.h',
  'gimpcolorprofilechooserdialog.h',
  'gimpcolorprofilecombobox.h',
  'gimpcolorprofilestore.h',
  'gimpcolorprofileview.h',
  'gimpcolorselection.h',
  'gimpcolorselector.h',
  'gimpdialog.h',
  'gimpenumcombobox.h',
  'gimpenumlabel.h',
  'gimpenumstore.h',
  'gimpenumwidgets.h',
  'gimpfileentry.h',
  'gimpframe.h',
  'gimphelpui.h',
  'gimphintbox.h',
  'gimpicons.h',
  'gimpintcombobox.h',
  'gimpintstore.h',
  'gimpmemsizeentry.h',
  'gimpnumberpairentry.h',
  'gimpoffsetarea.h',
  'gimppageselector.h',
  'gimppatheditor.h',
  'gimppickbutton.h',
  'gimppreview.h',
  'gimppreviewarea.h',
  'gimppropwidgets.h',
  'gimpquerybox.h',
  'gimpruler.h',
  'gimpscaleentry.h',
  'gimpscrolledpreview.h',
  'gimpsizeentry.h',
  'gimpspinbutton.h',
  'gimpstringcombobox.h',
  'gimpunitcombobox.h',
  'gimpunitstore.h',
  'gimpwidgets-error.h',
  'gimpwidgets.h',
  'gimpwidgetsenums.h',
  'gimpwidgetstypes.h',
  'gimpwidgetsutils.h',
  'gimpzoommodel.h',
)

libgimpwidgets_headers = [
  libgimpwidgets_headers_introspectable,
  'gimpcolorscale.h',
  'gimpcolorscales.h',
  'gimpcolorselect.h',
  'gimpcontroller.h',
]

libgimpwidgets_introspectable = [
  libgimpwidgets_sources_introspectable,
  libgimpwidgets_headers_introspectable,
]

if gtk3_macos.found()
  libgimpwidgets_sources += [
    'gimppickbutton-quartz.c',
  ]
else
  libgimpwidgets_sources += [
    'gimppickbutton-default.c',
    'gimppickbutton-kwin.c',
  ]
endif

libgimpwidgets = library('gimpwidgets-'+ gimp_api_version,
  libgimpwidgets_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gtk3, lcms, math
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpWidgets"', '-DGIMP_WIDGETS_COMPILATION', ],
  link_with: [
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
  ],
  vs_module_defs: 'gimpwidgets.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpwidgets_headers,
  subdir: gimp_api_name / 'libgimpwidgets',
)


# Test programs, not installed

test_preview_area = executable('test-preview-area',
  'test-preview-area.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  c_args: '-DG_LOG_DOMAIN="LibGimpWidgets"',
  link_with: [ libgimpwidgets, ],
  install: false,
  build_by_default: false,
)

test_eevl = executable('test-eevl',
  'test-eevl.c',
  include_directories: rootInclude,
  dependencies: [
    glib, gtk3,
  ],
  c_args: '-DG_LOG_DOMAIN="LibGimpWidgets"',
  link_with: [ libgimpwidgets, ],
  install: false,
)
