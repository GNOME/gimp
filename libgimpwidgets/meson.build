stamp_widgets_enums = custom_target('stamp-gimpwidgetsenums.h',
  input : [
    files(
      'gimpwidgetsenums.h'
    ),
  ],
  output: [ 'stamp-gimpwidgetsenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'gimpwidgets',
    '#include <gio/gio.h>\n'              +
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "libgimp/libgimp-intl.h"',
    libgimp_mkenums_dtails
  ],
  build_by_default: true
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
  'gimpcolorscale.c',
  'gimpcolorscaleentry.c',
  'gimpcolorselection.c',
  'gimpcolorselector.c',
  'gimpdialog.c',
  'gimpenumcombobox.c',
  'gimpenumlabel.c',
  'gimpenumstore.c',
  'gimpenumwidgets.c',
  'gimpfilechooser.c',
  'gimpframe.c',
  'gimphelpui.c',
  'gimphintbox.c',
  'gimpicons.c',
  'gimpintcombobox.c',
  'gimpintradioframe.c',
  'gimpintstore.c',
  'gimplabelcolor.c',
  'gimplabeled.c',
  'gimplabelintwidget.c',
  'gimplabelspin.c',
  'gimplabelstringwidget.c',
  'gimplabelentry.c',
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
  'gimpspinscale.c',
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
  'gimpcolorscales.c',
  'gimpcolorselect.c',
  'gimpcontroller.c',
  'gimpeevl.c',
  'gimpfileentry.c',
  'gimpwidgets-private.c',

  'gimpwidgetsenums.c',
  stamp_widgets_enums,

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
  'gimpcolorscale.h',
  'gimpcolorscaleentry.h',
  'gimpcolorselection.h',
  'gimpcolorselector.h',
  'gimpdialog.h',
  'gimpenumcombobox.h',
  'gimpenumlabel.h',
  'gimpenumstore.h',
  'gimpenumwidgets.h',
  'gimpfilechooser.h',
  'gimpframe.h',
  'gimphelpui.h',
  'gimphintbox.h',
  'gimpicons.h',
  'gimpintcombobox.h',
  'gimpintradioframe.h',
  'gimplabelcolor.h',
  'gimplabeled.h',
  'gimplabelintwidget.h',
  'gimplabelspin.h',
  'gimplabelentry.h',
  'gimplabelstringwidget.h',
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
  'gimpspinscale.h',
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
  'gimpcolorscales.h',
  'gimpcolorselect.h',
  'gimpcontroller.h',
]

libgimpwidgets_introspectable = [
  libgimpwidgets_sources_introspectable,
  libgimpwidgets_headers_introspectable,
]

if platform_osx
  libgimpwidgets_sources += [
    'gimppickbutton-quartz.c',
  ]
elif platform_windows
  libgimpwidgets_sources += [
    'gimppickbutton-win32.c',
  ]
else
  libgimpwidgets_sources += [
    'gimppickbutton-kwin.c',
    'gimppickbutton-xdg.c',
  ]
endif

libgimpwidgets = library('gimpwidgets-'+ gimp_api_version,
  libgimpwidgets_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gtk3, lcms, math, mscms
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
    glib, gtk3, math,
  ],
  c_args: '-DG_LOG_DOMAIN="LibGimpWidgets"',
  link_with: [ libgimpwidgets, ],
  install: false,
)
