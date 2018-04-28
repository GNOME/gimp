## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = ./libgimp-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
if HAVE_EXCHNDL
exchndl = -lexchndl
endif

gimp_def = gimp.def
gimpui_def = gimpui.def
libgimp_export_symbols = -export-symbols $(srcdir)/gimp.def
libgimpui_export_symbols = -export-symbols $(srcdir)/gimpui.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimp-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libgimpui-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimp.def $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimpui.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimp-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/libgimpui-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimp.def
	-rm $(DESTDIR)$(libdir)/gimpui.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimp-$(GIMP_API_VERSION).lib gimpui-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimp-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)
	$(INSTALL) gimpui-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimp-$(GIMP_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/gimpui-$(GIMP_API_VERSION).lib

gimp-@GIMP_API_VERSION@.lib: gimp.def
	lib -name:libgimp-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimp.def -out:$@

gimpui-@GIMP_API_VERSION@.lib: gimpui.def
	lib -name:libgimpui-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpui.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

gimpincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimp

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"				\
	-DGIMP_USER_VERSION=\"$(GIMP_USER_VERSION)\"		\
	-DG_LOG_DOMAIN=\"LibGimp\"	\
	-DGIMP_COMPILATION		\
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GEXIV2_CFLAGS)		\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

lib_LTLIBRARIES = libgimp-@GIMP_API_VERSION@.la libgimpui-@GIMP_API_VERSION@.la

PDB_WRAPPERS_C = \
	gimp_pdb.c			\
	gimpbrush_pdb.c			\
	gimpbrushes_pdb.c		\
	gimpbrushselect_pdb.c		\
	gimpbuffer_pdb.c		\
	gimpchannel_pdb.c		\
	gimpcontext_pdb.c		\
	gimpdebug_pdb.c			\
	gimpdisplay_pdb.c		\
	gimpdrawable_pdb.c		\
	gimpdrawablecolor_pdb.c		\
	gimpdrawableedit_pdb.c		\
	gimpdynamics_pdb.c		\
	gimpedit_pdb.c			\
	gimpfileops_pdb.c		\
	gimpfloatingsel_pdb.c		\
	gimpfonts_pdb.c			\
	gimpfontselect_pdb.c		\
	gimpgimprc_pdb.c		\
	gimpgradient_pdb.c		\
	gimpgradients_pdb.c		\
	gimpgradientselect_pdb.c	\
	gimphelp_pdb.c			\
	gimpimage_pdb.c			\
	gimpimagecolorprofile_pdb.c	\
	gimpimageconvert_pdb.c		\
	gimpimagegrid_pdb.c		\
	gimpimageguides_pdb.c		\
	gimpimagesamplepoints_pdb.c	\
	gimpimageselect_pdb.c		\
	gimpimagetransform_pdb.c	\
	gimpimageundo_pdb.c		\
	gimpitem_pdb.c			\
	gimpitemtransform_pdb.c		\
	gimplayer_pdb.c			\
	gimpmessage_pdb.c		\
	gimppainttools_pdb.c		\
	gimppalette_pdb.c		\
	gimppalettes_pdb.c		\
	gimppaletteselect_pdb.c		\
	gimppattern_pdb.c		\
	gimppatterns_pdb.c		\
	gimppatternselect_pdb.c		\
	gimpplugin_pdb.c		\
	gimpproceduraldb_pdb.c		\
	gimpprogress_pdb.c		\
	gimpselection_pdb.c		\
	gimptextlayer_pdb.c		\
	gimptexttool_pdb.c		\
	gimpunit_pdb.c			\
	gimpvectors_pdb.c

PDB_WRAPPERS_H = \
	gimp_pdb_headers.h		\
	gimp_pdb.h			\
	gimpbrush_pdb.h			\
	gimpbrushes_pdb.h		\
	gimpbrushselect_pdb.h		\
	gimpbuffer_pdb.h		\
	gimpchannel_pdb.h		\
	gimpcontext_pdb.h		\
	gimpdebug_pdb.h			\
	gimpdisplay_pdb.h		\
	gimpdrawable_pdb.h		\
	gimpdrawablecolor_pdb.h		\
	gimpdrawableedit_pdb.h		\
	gimpdynamics_pdb.h		\
	gimpedit_pdb.h			\
	gimpfileops_pdb.h		\
	gimpfloatingsel_pdb.h		\
	gimpfonts_pdb.h			\
	gimpfontselect_pdb.h		\
	gimpgimprc_pdb.h		\
	gimpgradient_pdb.h		\
	gimpgradients_pdb.h		\
	gimpgradientselect_pdb.h	\
	gimphelp_pdb.h			\
	gimpimage_pdb.h			\
	gimpimagecolorprofile_pdb.h	\
	gimpimageconvert_pdb.h		\
	gimpimagegrid_pdb.h		\
	gimpimageguides_pdb.h		\
	gimpimagesamplepoints_pdb.h	\
	gimpimageselect_pdb.h		\
	gimpimagetransform_pdb.h	\
	gimpimageundo_pdb.h		\
	gimpitem_pdb.h			\
	gimpitemtransform_pdb.h		\
	gimplayer_pdb.h			\
	gimpmessage_pdb.h		\
	gimppainttools_pdb.h		\
	gimppalette_pdb.h		\
	gimppalettes_pdb.h		\
	gimppaletteselect_pdb.h		\
	gimppattern_pdb.h		\
	gimppatterns_pdb.h		\
	gimppatternselect_pdb.h		\
	gimpplugin_pdb.h		\
	gimpproceduraldb_pdb.h		\
	gimpprogress_pdb.h		\
	gimpselection_pdb.h		\
	gimptextlayer_pdb.h		\
	gimptexttool_pdb.h		\
	gimpunit_pdb.h			\
	gimpvectors_pdb.h

libgimp_sources = \
	gimp.c			\
	gimp.h			\
	gimptypes.h		\
	gimpenums.h		\
	${PDB_WRAPPERS_C}	\
	${PDB_WRAPPERS_H}	\
	gimpbrushselect.c	\
	gimpbrushselect.h	\
	gimpchannel.c		\
	gimpchannel.h		\
	gimpdrawable.c		\
	gimpdrawable.h		\
	gimpfontselect.c	\
	gimpfontselect.h	\
	gimpgimprc.c		\
	gimpgimprc.h		\
	gimpgradientselect.c	\
	gimpgradientselect.h	\
	gimpimage.c		\
	gimpimage.h		\
	gimpimagecolorprofile.c	\
	gimpimagecolorprofile.h	\
	gimplayer.c		\
	gimplayer.h		\
	gimppaletteselect.c	\
	gimppaletteselect.h	\
	gimppatternselect.c	\
	gimppatternselect.h	\
	gimppixbuf.c		\
	gimppixbuf.h		\
	gimppixelfetcher.c	\
	gimppixelfetcher.h	\
	gimppixelrgn.c		\
	gimppixelrgn.h		\
	gimpplugin.c		\
	gimpplugin.h		\
	gimpproceduraldb.c	\
	gimpproceduraldb.h	\
	gimpprogress.c		\
	gimpprogress.h		\
	gimpregioniterator.c	\
	gimpregioniterator.h	\
	gimpselection.c		\
	gimpselection.h		\
	gimptile.c		\
	gimptile.h		\
	gimptilebackendplugin.c \
	gimptilebackendplugin.h \
	gimpunitcache.c		\
	gimpunitcache.h		\
	stdplugins-intl.h	\
	libgimp-intl.h

libgimp_built_sources = \
	gimpenums.c

libgimp_extra_sources = gimpenums.c.tail

libgimp_@GIMP_API_VERSION@_la_SOURCES = \
	$(libgimp_built_sources)	\
	$(libgimp_sources)

libgimpui_sources = \
	gimpui.c			\
	gimpui.h			\
	gimpuitypes.h			\
	gimpaspectpreview.c     	\
	gimpaspectpreview.h     	\
	gimpbrushselectbutton.c		\
	gimpbrushselectbutton.h		\
	gimpdrawablepreview.c		\
	gimpdrawablepreview.h		\
	gimpexport.c			\
	gimpexport.h			\
	gimpfontselectbutton.c		\
	gimpfontselectbutton.h		\
	gimpgradientselectbutton.c	\
	gimpgradientselectbutton.h	\
	gimpimagecombobox.c		\
	gimpimagecombobox.h		\
	gimpimagemetadata.c		\
	gimpimagemetadata.h		\
	gimpitemcombobox.c		\
	gimpitemcombobox.h		\
	gimppaletteselectbutton.c	\
	gimppaletteselectbutton.h	\
	gimppatternselectbutton.c	\
	gimppatternselectbutton.h	\
	gimpprocbrowserdialog.c		\
	gimpprocbrowserdialog.h		\
	gimpprocview.c			\
	gimpprocview.h			\
	gimpprogressbar.c		\
	gimpprogressbar.h		\
	gimpselectbutton.c		\
	gimpselectbutton.h		\
	gimpzoompreview.c		\
	gimpzoompreview.h

libgimpui_built_sources = \
	gimpuimarshal.c			\
	gimpuimarshal.h

libgimpui_extra_sources = gimpuimarshal.list

libgimpui_@GIMP_API_VERSION@_la_SOURCES = \
	$(libgimpui_built_sources)	\
	$(libgimpui_sources)

gimpinclude_HEADERS = \
	gimp.h				\
	gimptypes.h			\
	gimpenums.h			\
	${PDB_WRAPPERS_H}		\
	gimpbrushselect.h		\
	gimpchannel.h			\
	gimpdrawable.h			\
	gimpfontselect.h		\
	gimpgimprc.h			\
	gimpgradientselect.h		\
	gimpimage.h			\
	gimpimagecolorprofile.h		\
	gimplayer.h			\
	gimppaletteselect.h		\
	gimppatternselect.h		\
	gimppixelfetcher.h		\
	gimppixelrgn.h			\
	gimpplugin.h			\
	gimpproceduraldb.h		\
	gimpprogress.h			\
	gimpregioniterator.h		\
	gimpselection.h			\
	gimptile.h			\
	\
	gimpui.h			\
	gimpuitypes.h			\
	gimpaspectpreview.h		\
	gimpbrushselectbutton.h		\
	gimpdrawablepreview.h   	\
	gimpexport.h			\
	gimpfontselectbutton.h		\
	gimpgradientselectbutton.h	\
	gimpimagecombobox.h		\
	gimpimagemetadata.h		\
	gimpitemcombobox.h		\
	gimppaletteselectbutton.h	\
	gimppatternselectbutton.h	\
	gimppixbuf.h			\
	gimpprocbrowserdialog.h		\
	gimpprocview.h			\
	gimpprogressbar.h		\
	gimpselectbutton.h		\
	gimpzoompreview.h

libgimp_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimp_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libgimp_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimp_def)

libgimp_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(exchndl)		\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(RT_LIBS)

libgimpui_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimpui_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libgimpui_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimpui_def)

libgimpui_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimp)		\
	$(libgimpwidgets)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(libgimpmodule)	\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)

EXTRA_DIST = \
	COPYING				\
	gimp.def			\
	gimpui.def			\
	$(libgimp_extra_sources)	\
	$(libgimpui_extra_sources)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-cec xgen-umh xgen-umc
CLEANFILES = $(gen_sources)

gimpenums.c: $(srcdir)/gimpenums.h $(srcdir)/gimpenums.c.tail $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include <gegl.h>\n#undef GIMP_DISABLE_DEPRECATED\n#include \"libgimpbase/gimpbase.h\"\n#include \"libgimpbase/gimpbase-private.h\"\n#include \"libgimpconfig/gimpconfigenums.h\"\n#include \"gimpenums.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libgimp\");\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > xgen-cec \
	&& cat $(srcdir)/gimpenums.c.tail >> xgen-cec \
	&& cp xgen-cec $(@F) \
	&& rm -f xgen-cec

gimpuimarshal.h: $(srcdir)/gimpuimarshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_gimpui_marshal $(srcdir)/gimpuimarshal.list --header >> xgen-umh \
	&& (cmp -s xgen-umh $(@F) || cp xgen-umh $(@F)) \
	&& rm -f xgen-umh xgen-umh~

gimpuimarshal.c: gimpuimarshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_gimpui_marshal $(srcdir)/gimpuimarshal.list --header --body >> xgen-umc \
	&& cp xgen-umc $(@F) \
	&& rm -f xgen-umc xgen-umc~
