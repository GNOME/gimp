# Similarly to libgimpthumb/gimpthumb-enums.c, libgimp/gimpenums.c is
# not versioned so we don't use the mkenums_wrap.
# Moreover it is generated in 2 steps, first with common mkenums
# arguments, then concatenating with a "tail" file.
gimpenums_notail = custom_target('gimpenums.c.notail',
  input : [ 'gimpenums.h', stamp_enumcode],
  output: [ 'gimpenums.c.notail', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include <gegl.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "libgimpbase/gimpbase-private.h"\n'
            + '#include "libgimpconfig/gimpconfigenums.h"\n'
            + '#include "pdb/stamp-enumcode.h"\n'
            + '#include "gimpenums.h"\n',
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
      '  static const Gimp@Type@Desc descs[] =\n'+
      '  {',
    '--dprod',
      '    { @VALUENAME@, @valuedesc@, @valuehelp@ },'+
      '@if (\'@valueabbrev@\' ne \'NULL\')@\n'+
      '    /* Translators: this is an abbreviated version of @valueudesc@.\n'+
      '       Keep it short. */\n'+
      '    { @VALUENAME@, @valueabbrev@, NULL },'+
      '@endif@',
    '--dtail',
      libgimp_mkenums_dtails,
    '@INPUT0@',
  ],
  capture: true,
)

gimpenums = custom_target('gimpenums.c',
  input : [ gimpenums_notail, 'gimpenums.c.tail', ],
  output: [ 'gimpenums.c', ],
  command: [python, '-c', 'import sys; [sys.stdout.write(open(f).read()) for f in sys.argv[1:]]','@INPUT@'],
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
  'gimpdrawablefilter_pdb.c',
  'gimpdrawableselect_pdb.c',
  'gimpdynamics_pdb.c',
  'gimpedit_pdb.c',
  'gimpfile_pdb.c',
  'gimpfloatingsel_pdb.c',
  'gimpfont_pdb.c',
  'gimpfonts_pdb.c',
  'gimpfontselect_pdb.c',
  'gimpgimprc_pdb.c',
  'gimpgradient_pdb.c',
  'gimpgradients_pdb.c',
  'gimpgradientselect_pdb.c',
  'gimpgrouplayer_pdb.c',
  'gimphelp_pdb.c',
  'gimpimage_pdb.c',
  'gimpimageautocrop_pdb.c',
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
  'gimplinklayer_pdb.c',
  'gimpmessage_pdb.c',
  'gimppainttools_pdb.c',
  'gimppalette_pdb.c',
  'gimppalettes_pdb.c',
  'gimppaletteselect_pdb.c',
  'gimppath_pdb.c',
  'gimppattern_pdb.c',
  'gimppatterns_pdb.c',
  'gimppatternselect_pdb.c',
  'gimpprogress_pdb.c',
  'gimprasterizable_pdb.c',
  'gimpresource_pdb.c',
  'gimpselection_pdb.c',
  'gimptextlayer_pdb.c',
  'gimptexttool_pdb.c',
  'gimpunit_pdb.c',
  'gimpvectorlayer_pdb.c',
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
  'gimpdrawablefilter_pdb.h',
  'gimpdrawableselect_pdb.h',
  'gimpdynamics_pdb.h',
  'gimpedit_pdb.h',
  'gimpfile_pdb.h',
  'gimpfloatingsel_pdb.h',
  'gimpfont_pdb.h',
  'gimpfonts_pdb.h',
  'gimpfontselect_pdb.h',
  'gimpgimprc_pdb.h',
  'gimpgradient_pdb.h',
  'gimpgradients_pdb.h',
  'gimpgradientselect_pdb.h',
  'gimpgrouplayer_pdb.h',
  'gimphelp_pdb.h',
  'gimpimage_pdb.h',
  'gimpimageautocrop_pdb.h',
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
  'gimplinklayer_pdb.h',
  'gimpmessage_pdb.h',
  'gimppainttools_pdb.h',
  'gimppalette_pdb.h',
  'gimppalettes_pdb.h',
  'gimppaletteselect_pdb.h',
  'gimppath_pdb.h',
  'gimppattern_pdb.h',
  'gimppatterns_pdb.h',
  'gimppatternselect_pdb.h',
  'gimpprogress_pdb.h',
  'gimprasterizable_pdb.h',
  'gimpresource_pdb.h',
  'gimpselection_pdb.h',
  'gimptextlayer_pdb.h',
  'gimptexttool_pdb.h',
  'gimpunit_pdb.h',
  'gimpvectorlayer_pdb.h',
]

libgimp_sources_introspectable = [
  'gimp.c',
  'gimpbatchprocedure.c',
  'gimpbrush.c',
  'gimpchannel.c',
  'gimpdisplay.c',
  'gimpdrawable.c',
  'gimpdrawablefilter.c',
  'gimpdrawablefilterconfig.c',
  'gimpexportoptions.c',
  'gimpfileprocedure.c',
  'gimpfont.c',
  'gimpgimprc.c',
  'gimpgradient.c',
  'gimpgrouplayer.c',
  'gimpimage.c',
  'gimpimagecolorprofile.c',
  'gimpimagemetadata.c',
  'gimpimagemetadata-save.c',
  'gimpimageprocedure.c',
  'gimpitem.c',
  'gimplayer.c',
  'gimplayermask.c',
  'gimplinklayer.c',
  'gimploadprocedure.c',
  'gimppalette.c',
  'gimpparamspecs.c',
  'gimppath.c',
  'gimppattern.c',
  'gimppdb.c',
  'gimpplugin.c',
  'gimpprocedure.c',
  'gimpprocedure-params.c',
  'gimpprocedureconfig.c',
  'gimpprogress.c',
  'gimprasterizable.c',
  'gimpresource.c',
  'gimpexportprocedure.c',
  'gimpselection.c',
  'gimptextlayer.c',
  'gimpthumbnailprocedure.c',
  'gimpvectorlayer.c',
  'gimpvectorloadprocedure.c',
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
  'gimpresourceselect.c',
  'gimptilebackendplugin.c',
]

libgimp_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'gimp.h',
  'gimptypes.h',
  'gimpenums.h',

  # Other headers
  'gimpbatchprocedure.h',
  'gimpbrush.h',
  'gimpchannel.h',
  'gimpdisplay.h',
  'gimpdrawable.h',
  'gimpdrawablefilter.h',
  'gimpdrawablefilterconfig.h',
  'gimpexportoptions.h',
  'gimpfileprocedure.h',
  'gimpfont.h',
  'gimpgimprc.h',
  'gimpgradient.h',
  'gimpgrouplayer.h',
  'gimpimage.h',
  'gimpimagecolorprofile.h',
  'gimpimagemetadata.h',
  'gimpimageprocedure.h',
  'gimpitem.h',
  'gimplayer.h',
  'gimplayermask.h',
  'gimplinklayer.h',
  'gimploadprocedure.h',
  'gimppalette.h',
  'gimpparamspecs.h',
  'gimppath.h',
  'gimppattern.h',
  'gimppdb.h',
  'gimpplugin.h',
  'gimpprocedure.h',
  'gimpprocedure-params.h',
  'gimpprocedureconfig.h',
  'gimpprogress.h',
  'gimprasterizable.h',
  'gimpresource.h',
  'gimpexportprocedure.h',
  'gimpselection.h',
  'gimptextlayer.h',
  'gimpthumbnailprocedure.h',
  'gimpvectorlayer.h',
  'gimpvectorloadprocedure.h',
  pdb_wrappers_headers,
]

libgimp_headers = [
  libgimp_headers_introspectable,
]

libgimpui_sources_introspectable = [
  'gimpaspectpreview.c',
  'gimpbrushchooser.c',
  'gimpdrawablechooser.c',
  'gimpdrawablepreview.c',
  'gimpfontchooser.c',
  'gimpgradientchooser.c',
  'gimpimagecombobox.c',
  'gimpitemcombobox.c',
  'gimppalettechooser.c',
  'gimppatternchooser.c',
  'gimpprocbrowserdialog.c',
  'gimpproceduredialog.c',
  'gimpprocview.c',
  'gimpprogressbar.c',
  'gimppropwidgets.c',
  'gimpresourcechooser.c',
  'gimpexportproceduredialog.c',
  'gimpui.c',
  'gimpzoompreview.c',
  'gimpvectorloadproceduredialog.c',
]

libgimpui_sources = [
  libgimpui_sources_introspectable,
  gimpuimarshal,

  # TODO: eventually move to libgimpwidgets.
  'gimpresolutionentry-private.c'
]

libgimpui_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'gimpui.h',
  'gimpuitypes.h',

  # Other headers
  'gimpaspectpreview.h',
  'gimpbrushchooser.h',
  'gimpdrawablechooser.h',
  'gimpdrawablepreview.h',
  'gimpfontchooser.h',
  'gimpgradientchooser.h',
  'gimpimagecombobox.h',
  'gimpitemcombobox.h',
  'gimppalettechooser.h',
  'gimppatternchooser.h',
  'gimpprocbrowserdialog.h',
  'gimpproceduredialog.h',
  'gimpprocview.h',
  'gimpprogressbar.h',
  'gimppropwidgets.h',
  'gimpresourcechooser.h',
  'gimpexportproceduredialog.h',
  'gimpvectorloadproceduredialog.h',
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


if glib.version().version_compare('>=2.86')
  if not platform_windows
    gio_specific_gir = 'GioUnix-2.0'
    gio_specific_vapi = 'gio-unix-2.0'
  else
    gio_specific_gir = 'GioWin32-2.0'
    gio_specific_vapi = 'gio-windows-2.0'
  endif
else
  gio_specific_gir = ''
  gio_specific_vapi = ''
endif

libgimp_deps_table = [
    { 'gir': 'Babl-0.1',          'vapi': 'babl-0.1',        },
    { 'gir': 'cairo-1.0',         'vapi': 'cairo-1.0',       },
    { 'gir': 'GdkPixbuf-2.0',     'vapi': 'gdk-pixbuf-2.0',  },
    { 'gir': 'Gegl-0.4',          'vapi': 'gegl-0.4',        },
    { 'gir': 'Gio-2.0',           'vapi': 'gio-2.0',         },
    { 'gir': gio_specific_gir,    'vapi': gio_specific_vapi, },
    { 'gir': 'GLib-2.0',          'vapi': 'glib-2.0',        },
    { 'gir': 'GObject-2.0',       'vapi': 'gobject-2.0',     },
    { 'gir': 'Pango-1.0',         'vapi': 'gtk+-3.0',        },
  ]


libgimp = library('gimp-'+ gimp_api_version,
  libgimp_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo,
    gdk_pixbuf,
    gegl,
    gexiv2,
    drmingw,
    math,
    pango,
    pangoft2,
  ],
  sources: [
    pdbgen
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimp"', '-DGIMP_COMPILATION', ],
  link_with: [
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmodule,
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
    gexiv2,
  ],
  link_with: [
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpmodule,
  ],
  include_directories: rootInclude,
)


libgimpui = library('gimpui-'+ gimp_api_version,
  libgimpui_sources,
  dependencies: [
    libgimp_dep,
    gtk3,
  ],
  sources: [
    pdbgen
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpUI"', '-DGIMP_COMPILATION', ],
  link_with: [
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
  libgimpmodule_introspectable,
  'gimpparamspecs-body.c'
]

libgimpui_introspectable_files = [
  libgimpui_introspectable,
  libgimpwidgets_introspectable,
]

if meson.is_cross_build() and not get_option('can-crosscompile-gir')
  have_gobject_introspection = false
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is disabled while cross-compiling because',
    'GI tools do not manage to properly generate introspection data',
    'while cross-compiling. Nevertheless we consider our introspected',
    'API as a core component of GIMP, thus you are expected to build',
    'this data natively.',
    'To forcefully build with GObject Introspection, set:',
    '-Dcan-crosscompile-gir=true'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
elif get_option('b_sanitize') != 'none'
  have_gobject_introspection = false
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is disabled while building with a sanitizer.',
    'See: https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/375'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
else
  have_gobject_introspection = true
  libgimp_gir_includes = []
  foreach dep : libgimp_deps_table
    if dep['gir'] != ''
      libgimp_gir_includes += [dep['gir']]
    endif
  endforeach

  gir_args = [
    '-DGIMP_COMPILATION',
    '-DGIMP_BASE_COMPILATION',
    '-DGIMP_COLOR_COMPILATION',
    '-DGIMP_CONFIG_COMPILATION',
    '-DGIMP_MATH_COMPILATION',
    '-DGIMP_MODULE_COMPILATION',
    '-DGIMP_THUMB_COMPILATION',
    '-DGIMP_WIDGETS_COMPILATION',
    '--quiet',
  ]

  libgimp_gir = gnome.generate_gir(
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpmodule,

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
    libgimpui,
    libgimpwidgets,

    sources: libgimpui_introspectable_files,
    nsversion: gimp_api_version,
    namespace: 'GimpUi',
    identifier_prefix: 'Gimp',
    symbol_prefix: [ 'gimp_ui', 'gimp' ],
    export_packages: 'gimp-ui-' + gimp_api_version,
    header: 'libgimp/gimpui.h',
    includes: [ libgimp_gir_includes, 'Gtk-3.0', libgimp_gir[0] ],
    extra_args: gir_args,
    include_directories: rootInclude,
    install: true,
  )

  if platform_osx
    g_ir_compiler = find_program('g-ir-compiler')
    gimp_exe_typelib = custom_target('macos-typelib.stamp',
                                     output: [ 'macos-typelib.stamp' ],
                                     command: [
                                       'make-in-build-typelib.sh', libgimp_gir, libgimpui_gir,
                                       meson.current_build_dir(), prefix, g_ir_compiler
                                     ],
                                     build_by_default: true)
  endif

  if have_vala
    vapi_deps = []
    foreach dep : libgimp_deps_table
      if dep['vapi'] != ''
        vapi_deps += [dep['vapi']]
      endif
    endforeach

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

if not platform_windows and not platform_osx
  # Verify .def files for Windows.
  # Ironically we only check this on non-Windows platform, since the
  # script expects .so libraries, and I'm not sure that the `nm` tool is
  # available on Windows.
  # On the other hand, it is how it always has been, since we used to
  # only run this on dist step (which we historically run on a GNU/Linux
  # machine). And it worked fine until now.
  custom_target('check-def-files',
    input: [
      '../libgimpbase/gimpbase.def',
      '../libgimpcolor/gimpcolor.def',
      '../libgimpconfig/gimpconfig.def',
      '../libgimp/gimp.def',
      '../libgimp/gimpui.def',
      '../libgimpmath/gimpmath.def',
      '../libgimpmodule/gimpmodule.def',
      '../libgimpthumb/gimpthumb.def',
      '../libgimpwidgets/gimpwidgets.def',
    ],
    depends: [
      libgimpbase,
      libgimpcolor,
      libgimpconfig,
      libgimp,
      libgimpui,
      libgimpmath,
      libgimpmodule,
      libgimpthumb,
      libgimpwidgets
    ],
    output: [ 'check-def-files', ],
    command: [
      python, meson.project_source_root() / 'tools' / 'defcheck.py', meson.project_source_root(),
    ],
    build_by_default: true,
  )
endif
