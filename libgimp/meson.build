
gimpenums_notail = custom_target('gimpenums.c.notail',
  input : [ 'gimpenums.h', ],
  output: [ 'gimpenums.c.notail', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include <gegl.h>\n'
            + '#undef GIMP_DISABLE_DEPRECATED\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "libgimpbase/gimpbase-private.h"\n'
            + '#include "libgimpconfig/gimpconfigenums.h"\n'
            + '#include "gimpenums.h"\n',
    libgimp_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
)

gimpenums = custom_target('gimpenums.c',
  input : [ gimpenums_notail, 'gimpenums.c.tail', ],
  output: [ 'gimpenums.c', ],
  command: [ 'cat', '@INPUT@' ],
  capture: true,
)

gimpuimarshal = gnome.genmarshal('gimpuimarshal',
  prefix: '_gimpui_marshal',
  sources: 'gimpuimarshal.list',
  install_header: false,
)

pdb_wrappers_sources = [
  'gimp_pdb.c',
  'gimpbrush_pdb.c',
  'gimpbrushes_pdb.c',
  'gimpbrushselect_pdb.c',
  'gimpbuffer_pdb.c',
  'gimpchannel_pdb.c',
  'gimpcontext_pdb.c',
  'gimpdebug_pdb.c',
  'gimpdisplay_pdb.c',
  'gimpdrawable_pdb.c',
  'gimpdrawablecolor_pdb.c',
  'gimpdrawableedit_pdb.c',
  'gimpdynamics_pdb.c',
  'gimpedit_pdb.c',
  'gimpfile_pdb.c',
  'gimpfloatingsel_pdb.c',
  'gimpfonts_pdb.c',
  'gimpfontselect_pdb.c',
  'gimpgimprc_pdb.c',
  'gimpgradient_pdb.c',
  'gimpgradients_pdb.c',
  'gimpgradientselect_pdb.c',
  'gimphelp_pdb.c',
  'gimpimage_pdb.c',
  'gimpimagecolorprofile_pdb.c',
  'gimpimageconvert_pdb.c',
  'gimpimagegrid_pdb.c',
  'gimpimageguides_pdb.c',
  'gimpimagesamplepoints_pdb.c',
  'gimpimageselect_pdb.c',
  'gimpimagetransform_pdb.c',
  'gimpimageundo_pdb.c',
  'gimpitem_pdb.c',
  'gimpitemtransform_pdb.c',
  'gimplayer_pdb.c',
  'gimpmessage_pdb.c',
  'gimppainttools_pdb.c',
  'gimppalette_pdb.c',
  'gimppalettes_pdb.c',
  'gimppaletteselect_pdb.c',
  'gimppattern_pdb.c',
  'gimppatterns_pdb.c',
  'gimppatternselect_pdb.c',
  'gimpprogress_pdb.c',
  'gimpselection_pdb.c',
  'gimptextlayer_pdb.c',
  'gimptexttool_pdb.c',
  'gimpvectors_pdb.c',
]

pdb_wrappers_headers = [
  'gimp_pdb_headers.h',
  'gimp_pdb.h',
  'gimpbrush_pdb.h',
  'gimpbrushes_pdb.h',
  'gimpbrushselect_pdb.h',
  'gimpbuffer_pdb.h',
  'gimpchannel_pdb.h',
  'gimpcontext_pdb.h',
  'gimpdebug_pdb.h',
  'gimpdisplay_pdb.h',
  'gimpdrawable_pdb.h',
  'gimpdrawablecolor_pdb.h',
  'gimpdrawableedit_pdb.h',
  'gimpdynamics_pdb.h',
  'gimpedit_pdb.h',
  'gimpfile_pdb.h',
  'gimpfloatingsel_pdb.h',
  'gimpfonts_pdb.h',
  'gimpfontselect_pdb.h',
  'gimpgimprc_pdb.h',
  'gimpgradient_pdb.h',
  'gimpgradients_pdb.h',
  'gimpgradientselect_pdb.h',
  'gimphelp_pdb.h',
  'gimpimage_pdb.h',
  'gimpimagecolorprofile_pdb.h',
  'gimpimageconvert_pdb.h',
  'gimpimagegrid_pdb.h',
  'gimpimageguides_pdb.h',
  'gimpimagesamplepoints_pdb.h',
  'gimpimageselect_pdb.h',
  'gimpimagetransform_pdb.h',
  'gimpimageundo_pdb.h',
  'gimpitem_pdb.h',
  'gimpitemtransform_pdb.h',
  'gimplayer_pdb.h',
  'gimpmessage_pdb.h',
  'gimppainttools_pdb.h',
  'gimppalette_pdb.h',
  'gimppalettes_pdb.h',
  'gimppaletteselect_pdb.h',
  'gimppattern_pdb.h',
  'gimppatterns_pdb.h',
  'gimppatternselect_pdb.h',
  'gimpprogress_pdb.h',
  'gimpselection_pdb.h',
  'gimptextlayer_pdb.h',
  'gimptexttool_pdb.h',
  'gimpvectors_pdb.h',
]

libgimp_sources_introspectable = [
  'gimp.c',
  'gimpbrushselect.c',
  'gimpchannel.c',
  'gimpdisplay.c',
  'gimpdrawable.c',
  'gimpfileprocedure.c',
  'gimpfontselect.c',
  'gimpgimprc.c',
  'gimpgradientselect.c',
  'gimpimage.c',
  'gimpimagecolorprofile.c',
  'gimpimagemetadata-save.c',
  'gimpimageprocedure.c',
  'gimpitem.c',
  'gimplayer.c',
  'gimplayermask.c',
  'gimploadprocedure.c',
  'gimppaletteselect.c',
  'gimpparamspecs.c',
  'gimppatternselect.c',
  'gimppdb.c',
  'gimpplugin.c',
  'gimpprocedure.c',
  'gimpprocedureconfig.c',
  'gimpprogress.c',
  'gimpsaveprocedure.c',
  'gimpselection.c',
  'gimpthumbnailprocedure.c',
  'gimpvectors.c',
  gimpenums,
  pdb_wrappers_sources,
]

libgimp_sources = [
  libgimp_sources_introspectable,
  'gimp-debug.c',
  'gimp-shm.c',
  'gimpgpparams.c',
  'gimpparamspecs-desc.c',
  'gimppdb_pdb.c',
  'gimppdbprocedure.c',
  'gimppixbuf.c',
  'gimpplugin_pdb.c',
  'gimpunit_pdb.c',
  'gimpunitcache.c',
  'gimptilebackendplugin.c',
]

libgimp_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'gimp.h',
  'gimptypes.h',
  'gimpenums.h',

  # Other headers
  'gimpbrushselect.h',
  'gimpchannel.h',
  'gimpdisplay.h',
  'gimpdrawable.h',
  'gimpfileprocedure.h',
  'gimpfontselect.h',
  'gimpgimprc.h',
  'gimpgradientselect.h',
  'gimpimage.h',
  'gimpimagecolorprofile.h',
  'gimpimageprocedure.h',
  'gimpitem.h',
  'gimplayer.h',
  'gimplayermask.h',
  'gimploadprocedure.h',
  'gimppaletteselect.h',
  'gimpparamspecs.h',
  'gimppatternselect.h',
  'gimppdb.h',
  'gimpplugin.h',
  'gimpprocedure.h',
  'gimpprocedureconfig.h',
  'gimpprogress.h',
  'gimpsaveprocedure.h',
  'gimpselection.h',
  'gimpthumbnailprocedure.h',
  'gimpvectors.h',
  pdb_wrappers_headers,
]

libgimp_headers = [
  libgimp_headers_introspectable,
  'gimpprocedure-params.h',
]

libgimpui_sources_introspectable = [
  'gimpaspectpreview.c',
  'gimpbrushselectbutton.c',
  'gimpdrawablepreview.c',
  'gimpexport.c',
  'gimpfontselectbutton.c',
  'gimpgradientselectbutton.c',
  'gimpimagecombobox.c',
  'gimpimagemetadata.c',
  'gimpitemcombobox.c',
  'gimppaletteselectbutton.c',
  'gimppatternselectbutton.c',
  'gimpprocbrowserdialog.c',
  'gimpproceduredialog.c',
  'gimpprocview.c',
  'gimpprogressbar.c',
  'gimpselectbutton.c',
  'gimpui.c',
  'gimpzoompreview.c',
]

libgimpui_sources = [
  libgimpui_sources_introspectable,
  gimpuimarshal,
]

libgimpui_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'gimpui.h',
  'gimpuitypes.h',

  # Other headers
  'gimpaspectpreview.h',
  'gimpbrushselectbutton.h',
  'gimpdrawablepreview.h',
  'gimpexport.h',
  'gimpfontselectbutton.h',
  'gimpgradientselectbutton.h',
  'gimpimagecombobox.h',
  'gimpimagemetadata.h',
  'gimpitemcombobox.h',
  'gimppaletteselectbutton.h',
  'gimppatternselectbutton.h',
  'gimpprocbrowserdialog.h',
  'gimpproceduredialog.h',
  'gimpprocview.h',
  'gimpprogressbar.h',
  'gimpselectbutton.h',
  'gimpzoompreview.h',
]

libgimp_introspectable = [
  libgimp_sources_introspectable,
  libgimp_headers_introspectable,
]

libgimpui_introspectable = [
  libgimpui_sources_introspectable,
  libgimpui_headers_introspectable,
]


libgimp = library('gimp-'+ gimp_api_version,
  libgimp_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl,
    gexiv2,
    drmingw,
    gtk3,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimp"', '-DGIMP_COMPILATION', ],
  link_with: [
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
  ],
  vs_module_defs: 'gimp.def',
  install: true,
  version: so_version,
)

# Define an internal dependency for ease of reuse
libgimp_dep = declare_dependency(
  dependencies: [
    cairo,
    gdk_pixbuf,
    gegl,
  ],
  link_with: [
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
  ],
  include_directories: rootInclude,
)


libgimpui = library('gimpui-'+ gimp_api_version,
  libgimpui_sources,
  dependencies: [
    libgimp_dep,
    gexiv2,
    gtk3,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpUI"', '-DGIMP_COMPILATION', ],
  link_with: [
    libgimpmodule,
    libgimpwidgets,
  ],
  vs_module_defs: 'gimpui.def',
  install: true,
  version: so_version,
)

# Define an internal dependency for ease of reuse
libgimpui_dep = declare_dependency(
  dependencies: [
    libgimp_dep,
    gtk3,
  ],
  link_with: [
    libgimpmodule,
    libgimpui,
    libgimpwidgets,
  ],
  include_directories: rootInclude,
)


install_headers(
  libgimp_headers,
  libgimpui_headers_introspectable,
  subdir: gimp_api_name / 'libgimp',
)



### GObject introspection

libgimp_introspectable_files = [
  libgimp_introspectable,
  libgimpbase_introspectable,
  libgimpcolor_introspectable,
  libgimpconfig_introspectable,
  libgimpmath_introspectable,
  'gimpparamspecs-body.c',
]

libgimpui_introspectable_files = [
  libgimpmodule_introspectable,
  libgimpui_introspectable,
  libgimpwidgets_introspectable,
]

if meson.is_cross_build()
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is disabled while cross-compiling because',
    'GI tools do not manage to properly generate introspection data',
    'while cross-compiling. Nevertheless we consider our introspected',
    'API as a core component of GIMP, thus you are expected to build',
    'this data natively.'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
else
  libgimp_gir_includes = [
    'Babl-0.1',
    'cairo-1.0',
    'GdkPixbuf-2.0',
    'Gegl-0.4',
    'Gio-2.0',
    'GLib-2.0',
    'GObject-2.0',
    'Gtk-3.0',
  ]

  gir_args = [
    '-DGIMP_COMPILATION',
    '-DGIMP_BASE_COMPILATION',
    '-DGIMP_COLOR_COMPILATION',
    '-DGIMP_CONFIG_COMPILATION',
    '-DGIMP_MATH_COMPILATION',
    '-DGIMP_MODULE_COMPILATION',
    '-DGIMP_THUMB_COMPILATION',
    '-DGIMP_WIDGETS_COMPILATION',
    '-DGIMP_DISABLE_DEPRECATED',
    '--quiet',
  ]

  libgimp_gir = gnome.generate_gir(
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,

    sources: libgimp_introspectable_files,
    nsversion: gimp_api_version,
    namespace: 'Gimp',
    identifier_prefix: 'Gimp',
    symbol_prefix: 'gimp',
    export_packages: 'gimp-' + gimp_api_version,
    header: 'libgimp/gimp.h',
    includes: libgimp_gir_includes,
    extra_args: gir_args,
    include_directories: rootInclude,
    install: true,
  )

  libgimpui_gir = gnome.generate_gir(
    libgimpmodule,
    libgimpui,
    libgimpwidgets,

    sources: libgimpui_introspectable_files,
    nsversion: gimp_api_version,
    namespace: 'GimpUi',
    identifier_prefix: 'Gimp',
    symbol_prefix: 'gimp',
    export_packages: 'gimp-ui-' + gimp_api_version,
    header: 'libgimp/gimpui.h',
    includes: [ libgimp_gir_includes, libgimp_gir[0] ],
    extra_args: gir_args,
    include_directories: rootInclude,
    install: true,
  )

  if have_vala
    vapi_deps = [
      'babl-0.1',
      'cairo-1.0',
      'gdk-pixbuf-2.0',
      'gegl-0.4',
      'gio-2.0',
      'glib-2.0',
      'gobject-2.0',
      'gtk+-3.0',
    ]

    libgimp_vapi = gnome.generate_vapi('gimp-' + gimp_api_version,
      sources: libgimp_gir[0],
      packages: vapi_deps,
      install: true,
    )

    libgimpui_vapi = gnome.generate_vapi('gimp-ui-' + gimp_api_version,
      sources: libgimpui_gir[0],
      packages: vapi_deps + [ libgimp_vapi ],
      install: true,
    )
  endif
endif
