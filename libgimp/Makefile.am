## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = ./libgimp-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimp_def = gimp.def
gimpui_def = gimpui.def
libgimp_export_symbols = -export-symbols gimp.def
libgimpui_export_symbols = -export-symbols gimpui.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimp-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libgimpui-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimp.def $(DESTDIR)$(libdir)
	$(INSTALL) gimpui.def $(DESTDIR)$(libdir)

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
	-DG_LOG_DOMAIN=\"LibGimp\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

lib_LTLIBRARIES = libgimp-2.0.la libgimpui-2.0.la

PDB_WRAPPERS_C = \
	gimpbrush_pdb.c			\
	gimpbrushes_pdb.c		\
	gimpbrushselect_pdb.c		\
	gimpchannel_pdb.c		\
	gimpcolor_pdb.c			\
	gimpcontext_pdb.c		\
	gimpconvert_pdb.c		\
	gimpdisplay_pdb.c		\
	gimpdrawable_pdb.c		\
	gimpdrawabletransform_pdb.c	\
	gimpedit_pdb.c			\
	gimpfileops_pdb.c		\
	gimpfloatingsel_pdb.c		\
	gimpfonts_pdb.c			\
	gimpfontselect_pdb.c		\
	gimpgimprc_pdb.c		\
	gimpgradient_pdb.c		\
	gimpgradients_pdb.c		\
	gimpgradientselect_pdb.c	\
	gimpguides_pdb.c		\
	gimphelp_pdb.c			\
	gimpimage_pdb.c			\
	gimplayer_pdb.c			\
	gimpmessage_pdb.c		\
	gimpmisc_pdb.c			\
	gimppainttools_pdb.c		\
	gimppalette_pdb.c		\
	gimppalettes_pdb.c		\
	gimppaletteselect_pdb.c		\
	gimpparasite_pdb.c		\
	gimppaths_pdb.c			\
	gimppattern_pdb.c		\
	gimppatterns_pdb.c		\
	gimppatternselect_pdb.c		\
	gimpplugin_pdb.c		\
	gimpproceduraldb_pdb.c		\
	gimpprogress_pdb.c		\
	gimpselection_pdb.c		\
	gimpselectiontools_pdb.c	\
	gimptexttool_pdb.c		\
	gimptransformtools_pdb.c	\
	gimpundo_pdb.c			\
	gimpunit_pdb.c

PDB_WRAPPERS_H = \
	gimp_pdb.h			\
	gimpbrush_pdb.h			\
	gimpbrushes_pdb.h		\
	gimpbrushselect_pdb.h		\
	gimpchannel_pdb.h		\
	gimpcolor_pdb.h			\
	gimpcontext_pdb.h		\
	gimpconvert_pdb.h		\
	gimpdisplay_pdb.h		\
	gimpdrawable_pdb.h		\
	gimpdrawabletransform_pdb.h	\
	gimpedit_pdb.h			\
	gimpfileops_pdb.h		\
	gimpfloatingsel_pdb.h		\
	gimpfonts_pdb.h			\
	gimpfontselect_pdb.h		\
	gimpgimprc_pdb.h		\
	gimpgradient_pdb.h		\
	gimpgradients_pdb.h		\
	gimpgradientselect_pdb.h	\
	gimpguides_pdb.h		\
	gimphelp_pdb.h			\
	gimpimage_pdb.h			\
	gimplayer_pdb.h			\
	gimpmessage_pdb.h		\
	gimpmisc_pdb.h			\
	gimppainttools_pdb.h		\
	gimppalette_pdb.h		\
	gimppalettes_pdb.h		\
	gimppaletteselect_pdb.h		\
	gimpparasite_pdb.h		\
	gimppaths_pdb.h			\
	gimppattern_pdb.h		\
	gimppatterns_pdb.h		\
	gimppatternselect_pdb.h		\
	gimpplugin_pdb.h		\
	gimpproceduraldb_pdb.h		\
	gimpprogress_pdb.h		\
	gimpselection_pdb.h		\
	gimpselectiontools_pdb.h	\
	gimptexttool_pdb.h		\
	gimptransformtools_pdb.h	\
	gimpundo_pdb.h			\
	gimpunit_pdb.h

libgimp_2_0_la_sources = \
	gimp.c			\
	gimp.h			\
	gimpcompat.h		\
	gimptypes.h		\
	gimpenums.h		\
	${PDB_WRAPPERS_C}	\
	${PDB_WRAPPERS_H}	\
	gimpbrushes.c		\
	gimpbrushes.h		\
	gimpbrushselect.c	\
	gimpbrushselect.h	\
	gimpchannel.c		\
	gimpchannel.h		\
	gimpdrawable.c		\
	gimpdrawable.h		\
	gimpfontselect.c	\
	gimpfontselect.h	\
	gimpgradients.c		\
	gimpgradients.h		\
	gimpgradientselect.c	\
	gimpgradientselect.h	\
	gimpimage.c		\
	gimpimage.h		\
	gimplayer.c		\
	gimplayer.h		\
	gimppalette.c		\
	gimppalette.h		\
	gimppalettes.c		\
	gimppalettes.h		\
	gimppaletteselect.c	\
	gimppaletteselect.h	\
	gimppatterns.c		\
	gimppatterns.h		\
	gimppatternselect.c	\
	gimppatternselect.h	\
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
	gimpunitcache.c		\
	gimpunitcache.h		\
	stdplugins-intl.h	\
	libgimp-intl.h

libgimp_2_0_la_built_sources = \
	gimpenums.c

libgimp_2_0_la_extra_sources = gimpenums.c.tail

EXTRA_DIST = \
	COPYING				\
	makefile.msc			\
	gimp.def			\
	gimpui.def			\
	$(libgimp_2_0_la_extra_sources)

libgimp_2_0_la_SOURCES = \
	$(libgimp_2_0_la_sources)	\
	$(libgimp_2_0_la_built_sources)

libgimpui_2_0_la_SOURCES = \
	gimpui.c		\
	gimpui.h		\
	gimpuitypes.h		\
	gimpmenu.c		\
	gimpmenu.h		\
	gimpaspectpreview.c     \
	gimpaspectpreview.h     \
	gimpbrushmenu.c		\
	gimpbrushmenu.h		\
	gimpdrawablepreview.c	\
	gimpdrawablepreview.h	\
	gimpfontmenu.c		\
	gimpfontmenu.h		\
	gimpgradientmenu.c 	\
	gimpgradientmenu.h 	\
	gimppalettemenu.c	\
	gimppalettemenu.h	\
	gimppatternmenu.c	\
	gimppatternmenu.h	\
	gimpdrawablecombobox.c	\
	gimpdrawablecombobox.h	\
	gimpimagecombobox.c	\
	gimpimagecombobox.h	\
	gimppixbuf.c		\
	gimppixbuf.h		\
	gimpprogressbar.c	\
	gimpprogressbar.h	\
	gimpexport.c		\
	gimpexport.h

gimpinclude_HEADERS = \
	gimp.h			\
	gimpcompat.h		\
	gimptypes.h		\
	gimpenums.h		\
	${PDB_WRAPPERS_H}	\
	gimpbrushes.h		\
	gimpbrushselect.h	\
	gimpchannel.h		\
	gimpdrawable.h		\
	gimpfontselect.h	\
	gimpgradients.h		\
	gimpgradientselect.h	\
	gimpimage.h		\
	gimplayer.h		\
	gimppalette.h		\
	gimppalettes.h		\
	gimppaletteselect.h	\
	gimppatterns.h		\
	gimppatternselect.h	\
	gimppixelfetcher.h	\
	gimppixelrgn.h		\
	gimpplugin.h		\
	gimpproceduraldb.h	\
	gimpprogress.h		\
	gimpregioniterator.h	\
	gimpselection.h		\
	gimptile.h		\
	\
	gimpui.h		\
	gimpuitypes.h		\
	gimpmenu.h		\
	gimpaspectpreview.h	\
	gimpbrushmenu.h		\
	gimpdrawablepreview.h   \
	gimpfontmenu.h		\
	gimpgradientmenu.h	\
	gimppalettemenu.h	\
	gimppatternmenu.h	\
	gimppixbuf.h		\
	gimpprogressbar.h	\
	gimpdrawablecombobox.h	\
	gimpimagecombobox.h	\
	gimpexport.h

libgimp_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimp_export_symbols)

libgimp_2_0_la_LIBADD = $(libgimpcolor) $(libgimpbase) $(GLIB_LIBS) $(RT_LIBS)

libgimp_2_0_la_DEPENDENCIES = $(gimp_def) $(libgimpcolor) $(libgimpbase)

libgimpui_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimpui_export_symbols)

libgimpui_2_0_la_LIBADD = $(libgimp) $(libgimpwidgets) $(libgimpcolor) \
	 $(libgimpbase) $(libgimpmodule) $(GTK_LIBS) $(RT_LIBS)

libgimpui_2_0_la_DEPENDENCIES = $(gimpui_def) $(libgimp) $(libgimpwidgets) $(libgimpbase)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-cec
CLEANFILES = $(gen_sources)

$(srcdir)/gimpenums.c: $(srcdir)/gimpenums.h $(srcdir)/gimpenums.c.tail $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"gimpenums.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (! type)\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libgimp\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/gimpenums.h > xgen-cec \
	&& cat $(srcdir)/gimpenums.c.tail >> xgen-cec \
	&& cp xgen-cec $(@F) \
	&& rm -f xgen-cec
