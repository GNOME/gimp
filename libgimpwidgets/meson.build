stamp_widgets_enums = custom_target('stamp-ligmawidgetsenums.h',
  input : [
    files(
      'ligmawidgetsenums.h'
    ),
  ],
  output: [ 'stamp-ligmawidgetsenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'ligmawidgets',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "libligma/libligma-intl.h"',
    libligma_mkenums_dtails
  ],
  build_by_default: true
)

ligmawidgetsmarshal = gnome.genmarshal('ligmawidgetsmarshal',
  prefix: '_ligma_widgets_marshal',
  sources: 'ligmawidgetsmarshal.list',
  install_header: false,
)

libligmawidgets_sources_introspectable = files(
  'ligmabrowser.c',
  'ligmabusybox.c',
  'ligmabutton.c',
  'ligmacairo-utils.c',
  'ligmacellrenderercolor.c',
  'ligmacellrenderertoggle.c',
  'ligmachainbutton.c',
  'ligmacolorarea.c',
  'ligmacolorbutton.c',
  'ligmacolordisplay.c',
  'ligmacolordisplaystack.c',
  'ligmacolorhexentry.c',
  'ligmacolornotebook.c',
  'ligmacolorprofilechooserdialog.c',
  'ligmacolorprofilecombobox.c',
  'ligmacolorprofilestore.c',
  'ligmacolorprofileview.c',
  'ligmacolorscaleentry.c',
  'ligmacolorselection.c',
  'ligmacolorselector.c',
  'ligmadialog.c',
  'ligmaenumcombobox.c',
  'ligmaenumlabel.c',
  'ligmaenumstore.c',
  'ligmaenumwidgets.c',
  'ligmafileentry.c',
  'ligmaframe.c',
  'ligmahelpui.c',
  'ligmahintbox.c',
  'ligmaicons.c',
  'ligmaintcombobox.c',
  'ligmaintradioframe.c',
  'ligmaintstore.c',
  'ligmalabelcolor.c',
  'ligmalabeled.c',
  'ligmalabelintwidget.c',
  'ligmalabelspin.c',
  'ligmalabelentry.c',
  'ligmamemsizeentry.c',
  'ligmanumberpairentry.c',
  'ligmaoffsetarea.c',
  'ligmapageselector.c',
  'ligmapatheditor.c',
  'ligmapickbutton.c',
  'ligmapreview.c',
  'ligmapreviewarea.c',
  'ligmapropwidgets.c',
  'ligmaquerybox.c',
  'ligmaruler.c',
  'ligmascaleentry.c',
  'ligmascrolledpreview.c',
  'ligmasizeentry.c',
  'ligmaspinbutton.c',
  'ligmaspinscale.c',
  'ligmastringcombobox.c',
  'ligmaunitcombobox.c',
  'ligmaunitstore.c',
  'ligmawidgets-error.c',
  'ligmawidgets.c',
  'ligmawidgetsutils.c',
  'ligmazoommodel.c',
)

libligmawidgets_sources = [
  libligmawidgets_sources_introspectable,
  'ligmacolorscale.c',
  'ligmacolorscales.c',
  'ligmacolorselect.c',
  'ligmacontroller.c',
  'ligmaeevl.c',
  'ligmawidgets-private.c',

  'ligmawidgetsenums.c',
  stamp_widgets_enums,

  ligmawidgetsmarshal,
  icons_imgs_sources,
  cursors_sources,
  pickers_sources,
]

libligmawidgets_headers_introspectable = files(
  'ligmabrowser.h',
  'ligmabusybox.h',
  'ligmabutton.h',
  'ligmacairo-utils.h',
  'ligmacellrenderercolor.h',
  'ligmacellrenderertoggle.h',
  'ligmachainbutton.h',
  'ligmacolorarea.h',
  'ligmacolorbutton.h',
  'ligmacolordisplay.h',
  'ligmacolordisplaystack.h',
  'ligmacolorhexentry.h',
  'ligmacolornotebook.h',
  'ligmacolorprofilechooserdialog.h',
  'ligmacolorprofilecombobox.h',
  'ligmacolorprofilestore.h',
  'ligmacolorprofileview.h',
  'ligmacolorscaleentry.h',
  'ligmacolorselection.h',
  'ligmacolorselector.h',
  'ligmadialog.h',
  'ligmaenumcombobox.h',
  'ligmaenumlabel.h',
  'ligmaenumstore.h',
  'ligmaenumwidgets.h',
  'ligmafileentry.h',
  'ligmaframe.h',
  'ligmahelpui.h',
  'ligmahintbox.h',
  'ligmaicons.h',
  'ligmaintcombobox.h',
  'ligmaintradioframe.h',
  'ligmalabelcolor.h',
  'ligmalabeled.h',
  'ligmalabelintwidget.h',
  'ligmalabelspin.h',
  'ligmalabelentry.h',
  'ligmaintstore.h',
  'ligmamemsizeentry.h',
  'ligmanumberpairentry.h',
  'ligmaoffsetarea.h',
  'ligmapageselector.h',
  'ligmapatheditor.h',
  'ligmapickbutton.h',
  'ligmapreview.h',
  'ligmapreviewarea.h',
  'ligmapropwidgets.h',
  'ligmaquerybox.h',
  'ligmaruler.h',
  'ligmascaleentry.h',
  'ligmascrolledpreview.h',
  'ligmasizeentry.h',
  'ligmaspinbutton.h',
  'ligmaspinscale.h',
  'ligmastringcombobox.h',
  'ligmaunitcombobox.h',
  'ligmaunitstore.h',
  'ligmawidgets-error.h',
  'ligmawidgets.h',
  'ligmawidgetsenums.h',
  'ligmawidgetstypes.h',
  'ligmawidgetsutils.h',
  'ligmazoommodel.h',
)

libligmawidgets_headers = [
  libligmawidgets_headers_introspectable,
  'ligmacolorscale.h',
  'ligmacolorscales.h',
  'ligmacolorselect.h',
  'ligmacontroller.h',
]

libligmawidgets_introspectable = [
  libligmawidgets_sources_introspectable,
  libligmawidgets_headers_introspectable,
]

if gtk3_macos.found()
  libligmawidgets_sources += [
    'ligmapickbutton-quartz.c',
  ]
elif platform_windows
  libligmawidgets_sources += [
    'ligmapickbutton-win32.c',
  ]
else
  libligmawidgets_sources += [
    'ligmapickbutton-default.c',
    'ligmapickbutton-kwin.c',
    'ligmapickbutton-xdg.c',
  ]
endif

libligmawidgets = library('ligmawidgets-'+ ligma_api_version,
  libligmawidgets_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gtk3, lcms, math, mscms
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaWidgets"', '-DLIGMA_WIDGETS_COMPILATION', ],
  link_with: [
    libligmabase,
    libligmacolor,
    libligmaconfig,
  ],
  vs_module_defs: 'ligmawidgets.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmawidgets_headers,
  subdir: ligma_api_name / 'libligmawidgets',
)


# Test programs, not installed

test_preview_area = executable('test-preview-area',
  'test-preview-area.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  c_args: '-DG_LOG_DOMAIN="LibLigmaWidgets"',
  link_with: [ libligmawidgets, ],
  install: false,
  build_by_default: false,
)

test_eevl = executable('test-eevl',
  'test-eevl.c',
  include_directories: rootInclude,
  dependencies: [
    glib, gtk3, math,
  ],
  c_args: '-DG_LOG_DOMAIN="LibLigmaWidgets"',
  link_with: [ libligmawidgets, ],
  install: false,
)
