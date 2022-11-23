# Similarly to libligmathumb/ligmathumb-enums.c, libligma/ligmaenums.c is
# not versionned so we don't use the mkenums_wrap.
# Moreover it is generated in 2 steps, first with common mkenums
# arguments, then concatenating with a "tail" file.
ligmaenums_notail = custom_target('ligmaenums.c.notail',
  input : [ 'ligmaenums.h', ],
  output: [ 'ligmaenums.c.notail', ],
  command: [
    ligma_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include <gegl.h>\n'
            + '#undef LIGMA_DISABLE_DEPRECATED\n'
            + '#include "libligmabase/ligmabase.h"\n'
            + '#include "libligmabase/ligmabase-private.h"\n'
            + '#include "libligmaconfig/ligmaconfigenums.h"\n'
            + '#include "ligmaenums.h"\n',
    '--fprod',
      '/* enumerations from "@basename@" */',
    '--vhead',
      'GType\n'+
      '@enum_name@_get_type (void)\n'+
      '{\n'+
      '  static const G@Type@Value values[] =\n'+
      '  {',
    '--vprod',
      '    { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
    '--vtail',
      '    { 0, NULL, NULL }\n'+
      '  };\n',
    '--dhead',
      '  static const Ligma@Type@Desc descs[] =\n'+
      '  {',
    '--dprod',
      '    { @VALUENAME@, @valuedesc@, @valuehelp@ },'+
      '@if (\'@valueabbrev@\' ne \'NULL\')@\n'+
      '    /* Translators: this is an abbreviated version of @valueudesc@.\n'+
      '       Keep it short. */\n'+
      '    { @VALUENAME@, @valueabbrev@, NULL },'+
      '@endif@',
    '--dtail',
      libligma_mkenums_dtails,
    '@INPUT@',
  ],
  capture: true,
)

ligmaenums = custom_target('ligmaenums.c',
  input : [ ligmaenums_notail, 'ligmaenums.c.tail', ],
  output: [ 'ligmaenums.c', ],
  command: [ 'cat', '@INPUT@' ],
  capture: true,
)

ligmauimarshal = gnome.genmarshal('ligmauimarshal',
  prefix: '_ligmaui_marshal',
  sources: 'ligmauimarshal.list',
  install_header: false,
)

pdb_wrappers_sources = [
  'ligma_pdb.c',
  'ligmabrush_pdb.c',
  'ligmabrushes_pdb.c',
  'ligmabrushselect_pdb.c',
  'ligmabuffer_pdb.c',
  'ligmachannel_pdb.c',
  'ligmacontext_pdb.c',
  'ligmadebug_pdb.c',
  'ligmadisplay_pdb.c',
  'ligmadrawable_pdb.c',
  'ligmadrawablecolor_pdb.c',
  'ligmadrawableedit_pdb.c',
  'ligmadynamics_pdb.c',
  'ligmaedit_pdb.c',
  'ligmafile_pdb.c',
  'ligmafloatingsel_pdb.c',
  'ligmafonts_pdb.c',
  'ligmafontselect_pdb.c',
  'ligmaligmarc_pdb.c',
  'ligmagradient_pdb.c',
  'ligmagradients_pdb.c',
  'ligmagradientselect_pdb.c',
  'ligmahelp_pdb.c',
  'ligmaimage_pdb.c',
  'ligmaimagecolorprofile_pdb.c',
  'ligmaimageconvert_pdb.c',
  'ligmaimagegrid_pdb.c',
  'ligmaimageguides_pdb.c',
  'ligmaimagesamplepoints_pdb.c',
  'ligmaimageselect_pdb.c',
  'ligmaimagetransform_pdb.c',
  'ligmaimageundo_pdb.c',
  'ligmaitem_pdb.c',
  'ligmaitemtransform_pdb.c',
  'ligmalayer_pdb.c',
  'ligmamessage_pdb.c',
  'ligmapainttools_pdb.c',
  'ligmapalette_pdb.c',
  'ligmapalettes_pdb.c',
  'ligmapaletteselect_pdb.c',
  'ligmapattern_pdb.c',
  'ligmapatterns_pdb.c',
  'ligmapatternselect_pdb.c',
  'ligmaprogress_pdb.c',
  'ligmaselection_pdb.c',
  'ligmatextlayer_pdb.c',
  'ligmatexttool_pdb.c',
  'ligmavectors_pdb.c',
]

pdb_wrappers_headers = [
  'ligma_pdb_headers.h',
  'ligma_pdb.h',
  'ligmabrush_pdb.h',
  'ligmabrushes_pdb.h',
  'ligmabrushselect_pdb.h',
  'ligmabuffer_pdb.h',
  'ligmachannel_pdb.h',
  'ligmacontext_pdb.h',
  'ligmadebug_pdb.h',
  'ligmadisplay_pdb.h',
  'ligmadrawable_pdb.h',
  'ligmadrawablecolor_pdb.h',
  'ligmadrawableedit_pdb.h',
  'ligmadynamics_pdb.h',
  'ligmaedit_pdb.h',
  'ligmafile_pdb.h',
  'ligmafloatingsel_pdb.h',
  'ligmafonts_pdb.h',
  'ligmafontselect_pdb.h',
  'ligmaligmarc_pdb.h',
  'ligmagradient_pdb.h',
  'ligmagradients_pdb.h',
  'ligmagradientselect_pdb.h',
  'ligmahelp_pdb.h',
  'ligmaimage_pdb.h',
  'ligmaimagecolorprofile_pdb.h',
  'ligmaimageconvert_pdb.h',
  'ligmaimagegrid_pdb.h',
  'ligmaimageguides_pdb.h',
  'ligmaimagesamplepoints_pdb.h',
  'ligmaimageselect_pdb.h',
  'ligmaimagetransform_pdb.h',
  'ligmaimageundo_pdb.h',
  'ligmaitem_pdb.h',
  'ligmaitemtransform_pdb.h',
  'ligmalayer_pdb.h',
  'ligmamessage_pdb.h',
  'ligmapainttools_pdb.h',
  'ligmapalette_pdb.h',
  'ligmapalettes_pdb.h',
  'ligmapaletteselect_pdb.h',
  'ligmapattern_pdb.h',
  'ligmapatterns_pdb.h',
  'ligmapatternselect_pdb.h',
  'ligmaprogress_pdb.h',
  'ligmaselection_pdb.h',
  'ligmatextlayer_pdb.h',
  'ligmatexttool_pdb.h',
  'ligmavectors_pdb.h',
]

libligma_sources_introspectable = [
  'ligma.c',
  'ligmabatchprocedure.c',
  'ligmabrushselect.c',
  'ligmachannel.c',
  'ligmadisplay.c',
  'ligmadrawable.c',
  'ligmafileprocedure.c',
  'ligmafontselect.c',
  'ligmaligmarc.c',
  'ligmagradientselect.c',
  'ligmaimage.c',
  'ligmaimagecolorprofile.c',
  'ligmaimagemetadata.c',
  'ligmaimagemetadata-save.c',
  'ligmaimageprocedure.c',
  'ligmaitem.c',
  'ligmalayer.c',
  'ligmalayermask.c',
  'ligmaloadprocedure.c',
  'ligmapaletteselect.c',
  'ligmaparamspecs.c',
  'ligmapatternselect.c',
  'ligmapdb.c',
  'ligmaplugin.c',
  'ligmaprocedure.c',
  'ligmaprocedureconfig.c',
  'ligmaprogress.c',
  'ligmasaveprocedure.c',
  'ligmaselection.c',
  'ligmatextlayer.c',
  'ligmathumbnailprocedure.c',
  'ligmavectors.c',
  ligmaenums,
  pdb_wrappers_sources,
]

libligma_sources = [
  libligma_sources_introspectable,
  'ligma-debug.c',
  'ligma-shm.c',
  'ligmagpparams.c',
  'ligmaparamspecs-desc.c',
  'ligmapdb_pdb.c',
  'ligmapdbprocedure.c',
  'ligmapixbuf.c',
  'ligmaplugin_pdb.c',
  'ligmaunit_pdb.c',
  'ligmaunitcache.c',
  'ligmatilebackendplugin.c',
]

libligma_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'ligma.h',
  'ligmatypes.h',
  'ligmaenums.h',

  # Other headers
  'ligmabatchprocedure.h',
  'ligmabrushselect.h',
  'ligmachannel.h',
  'ligmadisplay.h',
  'ligmadrawable.h',
  'ligmafileprocedure.h',
  'ligmafontselect.h',
  'ligmaligmarc.h',
  'ligmagradientselect.h',
  'ligmaimage.h',
  'ligmaimagecolorprofile.h',
  'ligmaimagemetadata.h',
  'ligmaimageprocedure.h',
  'ligmaitem.h',
  'ligmalayer.h',
  'ligmalayermask.h',
  'ligmaloadprocedure.h',
  'ligmapaletteselect.h',
  'ligmaparamspecs.h',
  'ligmapatternselect.h',
  'ligmapdb.h',
  'ligmaplugin.h',
  'ligmaprocedure.h',
  'ligmaprocedureconfig.h',
  'ligmaprogress.h',
  'ligmasaveprocedure.h',
  'ligmaselection.h',
  'ligmatextlayer.h',
  'ligmathumbnailprocedure.h',
  'ligmavectors.h',
  pdb_wrappers_headers,
]

libligma_headers = [
  libligma_headers_introspectable,
  'ligmaprocedure-params.h',
]

libligmaui_sources_introspectable = [
  'ligmaaspectpreview.c',
  'ligmabrushselectbutton.c',
  'ligmadrawablepreview.c',
  'ligmaexport.c',
  'ligmafontselectbutton.c',
  'ligmagradientselectbutton.c',
  'ligmaimagecombobox.c',
  'ligmaitemcombobox.c',
  'ligmapaletteselectbutton.c',
  'ligmapatternselectbutton.c',
  'ligmaprocbrowserdialog.c',
  'ligmaproceduredialog.c',
  'ligmaprocview.c',
  'ligmaprogressbar.c',
  'ligmasaveproceduredialog.c',
  'ligmaselectbutton.c',
  'ligmaui.c',
  'ligmazoompreview.c',
]

libligmaui_sources = [
  libligmaui_sources_introspectable,
  ligmauimarshal,
]

libligmaui_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'ligmaui.h',
  'ligmauitypes.h',

  # Other headers
  'ligmaaspectpreview.h',
  'ligmabrushselectbutton.h',
  'ligmadrawablepreview.h',
  'ligmaexport.h',
  'ligmafontselectbutton.h',
  'ligmagradientselectbutton.h',
  'ligmaimagecombobox.h',
  'ligmaitemcombobox.h',
  'ligmapaletteselectbutton.h',
  'ligmapatternselectbutton.h',
  'ligmaprocbrowserdialog.h',
  'ligmaproceduredialog.h',
  'ligmaprocview.h',
  'ligmaprogressbar.h',
  'ligmasaveproceduredialog.h',
  'ligmaselectbutton.h',
  'ligmazoompreview.h',
]

libligma_introspectable = [
  libligma_sources_introspectable,
  libligma_headers_introspectable,
]

libligmaui_introspectable = [
  libligmaui_sources_introspectable,
  libligmaui_headers_introspectable,
]


libligma = library('ligma-'+ ligma_api_version,
  libligma_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl,
    gexiv2,
    drmingw,
    gtk3,
  ],
  sources: [
    pdbgen
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigma"', '-DLIGMA_COMPILATION', ],
  link_with: [
    libligmabase,
    libligmacolor,
    libligmaconfig,
    libligmamodule,
  ],
  vs_module_defs: 'ligma.def',
  install: true,
  version: so_version,
)

# Define an internal dependency for ease of reuse
libligma_dep = declare_dependency(
  dependencies: [
    cairo,
    gdk_pixbuf,
    gegl,
  ],
  link_with: [
    libligma,
    libligmabase,
    libligmacolor,
    libligmaconfig,
    libligmamath,
    libligmamodule,
  ],
  include_directories: rootInclude,
)


libligmaui = library('ligmaui-'+ ligma_api_version,
  libligmaui_sources,
  dependencies: [
    libligma_dep,
    gexiv2,
    gtk3,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaUI"', '-DLIGMA_COMPILATION', ],
  link_with: [
    libligmawidgets,
  ],
  vs_module_defs: 'ligmaui.def',
  install: true,
  version: so_version,
)

# Define an internal dependency for ease of reuse
libligmaui_dep = declare_dependency(
  dependencies: [
    libligma_dep,
    gtk3,
  ],
  link_with: [
    libligmaui,
    libligmawidgets,
  ],
  include_directories: rootInclude,
)


install_headers(
  libligma_headers,
  libligmaui_headers_introspectable,
  subdir: ligma_api_name / 'libligma',
)



### GObject introspection

libligma_introspectable_files = [
  libligma_introspectable,
  libligmabase_introspectable,
  libligmacolor_introspectable,
  libligmaconfig_introspectable,
  libligmamath_introspectable,
  libligmamodule_introspectable,
  'ligmaparamspecs-body.c',
]

libligmaui_introspectable_files = [
  libligmaui_introspectable,
  libligmawidgets_introspectable,
]

if meson.is_cross_build() and not get_option('can-crosscompile-gir')
  have_gobject_introspection = false
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is disabled while cross-compiling because',
    'GI tools do not manage to properly generate introspection data',
    'while cross-compiling. Nevertheless we consider our introspected',
    'API as a core component of LIGMA, thus you are expected to build',
    'this data natively.',
    'To forcefully build with GObject Introspection, set:',
    '-Dcan-crosscompile-gir=true'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
else
  have_gobject_introspection = true
  libligma_gir_includes = [
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
    '-DLIGMA_COMPILATION',
    '-DLIGMA_BASE_COMPILATION',
    '-DLIGMA_COLOR_COMPILATION',
    '-DLIGMA_CONFIG_COMPILATION',
    '-DLIGMA_MATH_COMPILATION',
    '-DLIGMA_MODULE_COMPILATION',
    '-DLIGMA_THUMB_COMPILATION',
    '-DLIGMA_WIDGETS_COMPILATION',
    '-DLIGMA_DISABLE_DEPRECATED',
    '--quiet',
  ]

  libligma_gir = gnome.generate_gir(
    libligma,
    libligmabase,
    libligmacolor,
    libligmaconfig,
    libligmamath,
    libligmamodule,

    sources: libligma_introspectable_files,
    nsversion: ligma_api_version,
    namespace: 'Ligma',
    identifier_prefix: 'Ligma',
    symbol_prefix: 'ligma',
    export_packages: 'ligma-' + ligma_api_version,
    header: 'libligma/ligma.h',
    includes: libligma_gir_includes,
    extra_args: gir_args,
    include_directories: rootInclude,
    install: true,
  )

  libligmaui_gir = gnome.generate_gir(
    libligmaui,
    libligmawidgets,

    sources: libligmaui_introspectable_files,
    nsversion: ligma_api_version,
    namespace: 'LigmaUi',
    identifier_prefix: 'Ligma',
    symbol_prefix: [ 'ligma_ui', 'ligma' ],
    export_packages: 'ligma-ui-' + ligma_api_version,
    header: 'libligma/ligmaui.h',
    includes: [ libligma_gir_includes, libligma_gir[0] ],
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

    libligma_vapi = gnome.generate_vapi('ligma-' + ligma_api_version,
      sources: libligma_gir[0],
      packages: vapi_deps,
      install: true,
    )

    libligmaui_vapi = gnome.generate_vapi('ligma-ui-' + ligma_api_version,
      sources: libligmaui_gir[0],
      packages: vapi_deps + [ libligma_vapi ],
      install: true,
    )
  endif
endif

if have_python and not platform_windows and not platform_osx
  # Verify .def files for Windows.
  # Ironically we only check this on non-Windows platform, since the
  # script expects .so libraries, and I'm not sure that the `nm` tool is
  # available on Windows.
  # On the other hand, it is how it always has been, since we used to
  # only run this on dist step (which we historically run on a GNU/Linux
  # machine). And it worked fine until now.
  custom_target('check-def-files',
    input: [
      '../libligmabase/ligmabase.def',
      '../libligmacolor/ligmacolor.def',
      '../libligmaconfig/ligmaconfig.def',
      '../libligma/ligma.def',
      '../libligma/ligmaui.def',
      '../libligmamath/ligmamath.def',
      '../libligmamodule/ligmamodule.def',
      '../libligmathumb/ligmathumb.def',
      '../libligmawidgets/ligmawidgets.def',
    ],
    depends: [
      libligmabase,
      libligmacolor,
      libligmaconfig,
      libligma,
      libligmaui,
      libligmamath,
      libligmamodule,
      libligmathumb,
      libligmawidgets
    ],
    output: [ 'check-def-files', ],
    command: [
      python, meson.project_source_root() / 'tools' / 'defcheck.py', meson.project_source_root(),
    ],
    build_by_default: true,
  )
endif
