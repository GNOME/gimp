
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
    gimp_mkenums_custom_target_commonargs,
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
  'gimppdb_pdb.c',
  'gimppdbprocedure.c',
  'gimppixbuf.c',
  'gimpplugin_pdb.c',
  'gimpunit_pdb.c',
  'gimpunitcache.c',
  'gimptilebackendplugin.c',
]

libgimp_headers_introspectable = [
  'gimp.h',
  'gimpbrushselect.h',
  'gimpchannel.h',
  'gimpdisplay.h',
  'gimpdrawable.h',
  'gimpenums.h',
  'gimpfileprocedure.h',
  'gimpfontselect.h',
  'gimpgimprc.h',
  'gimpgradientselect.h',
  'gimpimage.h',
  'gimpimagecolorprofile.h',
  'gimpimageprocedure.h',
  'gimplayer.h',
  'gimplayermask.h',
  'gimploadprocedure.h',
  'gimppaletteselect.h',
  'gimpparamspecs.h',
  'gimppatternselect.h',
  'gimppdb.h',
  'gimpplugin.h',
  'gimpprocedure.h',
  'gimpprogress.h',
  'gimpsaveprocedure.h',
  'gimpselection.h',
  'gimpthumbnailprocedure.h',
  'gimptypes.h',
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
  'gimpprocview.h',
  'gimpprogressbar.h',
  'gimpselectbutton.h',
  'gimpui.h',
  'gimpuitypes.h',
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
    gegl, gexiv2, gtk3, drmingw,
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


libgimpui = library('gimpui-'+ gimp_api_version,
  libgimpui_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gtk3,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpUI"', '-DGIMP_COMPILATION', ],
  link_with: [
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpmodule,
    libgimpwidgets,
  ],
  vs_module_defs: 'gimpui.def',
  install: true,
  version: so_version,
)


install_headers(
  libgimp_headers,
  libgimpui_headers_introspectable,
  subdir: gimp_api_name / 'libgimp',
)



### GObject introspection

introspectable_files = [
  libgimp_introspectable,
  libgimpui_introspectable,
  libgimpbase_introspectable,
  libgimpcolor_introspectable,
  libgimpconfig_introspectable,
  libgimpmath_introspectable,
  libgimpmodule_introspectable,
  libgimpthumb_introspectable,
  libgimpwidgets_introspectable,
  'gimpparamspecs-body.c',
]

libgimp_gir = gnome.generate_gir(
  libgimp,
  libgimpui,
  libgimpbase,
  libgimpcolor,
  libgimpconfig,
  libgimpmath,
  libgimpmodule,
  libgimpthumb,
  libgimpwidgets,

  sources: introspectable_files,
  nsversion: gimp_api_version,
  namespace: 'Gimp',
  identifier_prefix: 'Gimp',
  # symbol_prefix: 'gimp',

  extra_args: [
    '-DGIMP_COMPILATION',
    '-DGIMP_BASE_COMPILATION',
    '-DGIMP_COLOR_COMPILATION',
    '-DGIMP_CONFIG_COMPILATION',
    '-DGIMP_MATH_COMPILATION',
    '-DGIMP_MODULE_COMPILATION',
    '-DGIMP_THUMB_COMPILATION',
    '-DGIMP_WIDGETS_COMPILATION',
  ],

  includes: [
    'Babl-0.1',
    'cairo-1.0',
    'GdkPixbuf-2.0',
    'Gegl-0.4',
    'Gio-2.0',
    'GLib-2.0',
    'GObject-2.0',
    'Gtk-3.0',
  ],

  link_with: [
    libgimpthumb,
  ],

  include_directories: rootInclude,
  install: true,
)
