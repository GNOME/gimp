## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(LT_RELEASE).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la
libgimp = ./libgimp-$(LT_RELEASE).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimp_def = gimp.def
gimpui_def = gimpui.def
libgimp_export_symbols = -export-symbols gimp.def
libgimpui_export_symbols = -export-symbols gimpui.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimp-$(LT_RELEASE).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libgimpui-$(LT_RELEASE).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimp.def $(DESTDIR)$(libdir)
	$(INSTALL) gimpui.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimp-$(LT_RELEASE).dll.a
	-rm $(DESTDIR)$(libdir)/libgimpui-$(LT_RELEASE).dll.a
	-rm $(DESTDIR)$(libdir)/gimp.def
	-rm $(DESTDIR)$(libdir)/gimpui.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimp-$(LT_RELEASE).lib gimpui-$(LT_RELEASE).lib 

install-ms-lib:
	$(INSTALL) gimp-$(LT_RELEASE).lib $(DESTDIR)$(libdir)
	$(INSTALL) gimpui-$(LT_RELEASE).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimp-$(LT_RELEASE).lib
	-rm $(DESTDIR)$(libdir)/gimpui-$(LT_RELEASE).lib

gimp-@LT_RELEASE@.lib: gimp.def
	lib -name:libgimp-$(LT_RELEASE)-@LT_CURRENT_MINUS_AGE@.dll -def:gimp.def -out:$@

gimpui-@LT_RELEASE@.lib: gimpui.def
	lib -name:libgimpui-$(LT_RELEASE)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpui.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

gimpincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimp

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimp\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	COPYING		\
	makefile.msc	\
	gimp.def	\
	gimpui.def

lib_LTLIBRARIES = libgimp-1.3.la libgimpui-1.3.la

PDB_WRAPPERS_C = \
	gimpbrushes_pdb.c		\
	gimpbrushselect_pdb.c		\
	gimpchannel_pdb.c		\
	gimpcolor_pdb.c			\
	gimpconvert_pdb.c		\
	gimpdisplay_pdb.c		\
	gimpdrawable_pdb.c		\
	gimpedit_pdb.c			\
	gimpfileops_pdb.c		\
	gimpfloatingsel_pdb.c		\
	gimpfonts_pdb.c			\
	gimpfontselect_pdb.c		\
	gimpgimprc_pdb.c		\
	gimpgradients_pdb.c		\
	gimpgradientselect_pdb.c	\
	gimpguides_pdb.c		\
	gimphelp_pdb.c			\
	gimpimage_pdb.c			\
	gimplayer_pdb.c			\
	gimpmessage_pdb.c		\
	gimpmisc_pdb.c			\
	gimpmisctools_pdb.c		\
	gimppainttools_pdb.c		\
	gimppalette_pdb.c		\
	gimppalettes_pdb.c		\
	gimppaletteselect_pdb.c		\
	gimpparasite_pdb.c		\
	gimppaths_pdb.c			\
	gimppatterns_pdb.c		\
	gimppatternselect_pdb.c		\
	gimpplugin_pdb.c		\
	gimpproceduraldb_pdb.c		\
	gimpselection_pdb.c		\
	gimpselectiontools_pdb.c	\
	gimptexttool_pdb.c		\
	gimptransformtools_pdb.c	\
	gimpundo_pdb.c			\
	gimpunit_pdb.c

PDB_WRAPPERS_H = \
	gimp_pdb.h			\
	gimpbrushes_pdb.h		\
	gimpbrushselect_pdb.h		\
	gimpchannel_pdb.h		\
	gimpcolor_pdb.h			\
	gimpconvert_pdb.h		\
	gimpdisplay_pdb.h		\
	gimpdrawable_pdb.h		\
	gimpedit_pdb.h			\
	gimpfileops_pdb.h		\
	gimpfloatingsel_pdb.h		\
	gimpfonts_pdb.h			\
	gimpfontselect_pdb.h		\
	gimpgimprc_pdb.h		\
	gimpgradients_pdb.h		\
	gimpgradientselect_pdb.h	\
	gimpguides_pdb.h		\
	gimphelp_pdb.h			\
	gimpimage_pdb.h			\
	gimplayer_pdb.h			\
	gimpmessage_pdb.h		\
	gimpmisc_pdb.h			\
	gimpmisctools_pdb.h		\
	gimppainttools_pdb.h		\
	gimppalette_pdb.h		\
	gimppalettes_pdb.h		\
	gimppaletteselect_pdb.h		\
	gimpparasite_pdb.h		\
	gimppaths_pdb.h			\
	gimppatterns_pdb.h		\
	gimppatternselect_pdb.h		\
	gimpplugin_pdb.h		\
	gimpproceduraldb_pdb.h		\
	gimpselection_pdb.h		\
	gimpselectiontools_pdb.h	\
	gimptexttool_pdb.h		\
	gimptransformtools_pdb.h	\
	gimpundo_pdb.h			\
	gimpunit_pdb.h

libgimp_1_3_la_SOURCES = \
	gimp.c			\
	gimp.h			\
	gimpcompat.h		\
	gimptypes.h		\
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
	gimpgradientselect.c	\
	gimpgradientselect.h	\
	gimpimage.c		\
	gimpimage.h		\
	gimplayer.c		\
	gimplayer.h		\
	gimpmisc.c		\
	gimpmisc.h		\
	gimppatternselect.c	\
	gimppatternselect.h	\
	gimppixelrgn.c		\
	gimppixelrgn.h		\
	gimpproceduraldb.c	\
	gimpproceduraldb.h	\
	gimpselection.c		\
	gimpselection.h		\
	gimptile.c		\
	gimptile.h		\
	gimpunitcache.c		\
	gimpunitcache.h		\
	stdplugins-intl.h	\
	libgimp-intl.h

libgimpui_1_3_la_SOURCES = \
	gimpui.c		\
	gimpui.h		\
	gimpuitypes.h		\
	gimpmenu.c		\
	gimpmenu.h		\
	gimpmiscui.c		\
	gimpmiscui.h		\
	gimpbrushmenu.c		\
	gimpbrushmenu.h		\
	gimpfontmenu.c		\
	gimpfontmenu.h		\
	gimpgradientmenu.c 	\
	gimpgradientmenu.h 	\
	gimppatternmenu.c	\
	gimppatternmenu.h	\
	gimpexport.c		\
	gimpexport.h

gimpinclude_HEADERS = \
	gimp.h			\
	gimpcompat.h		\
	gimptypes.h		\
	${PDB_WRAPPERS_H}	\
	gimpbrushselect.h	\
	gimpchannel.h		\
	gimpdrawable.h		\
	gimpenums.h		\
	gimpfontselect.h	\
	gimpgradientselect.h	\
	gimpimage.h		\
	gimplayer.h		\
	gimpmisc.h		\
	gimppatternselect.h	\
	gimppixelrgn.h		\
	gimpproceduraldb.h	\
	gimpselection.h		\
	gimptile.h		\
	\
	gimpexport.h		\
	gimpmiscui.h		\
	gimpui.h		\
	gimpuitypes.h		\
	gimpmenu.h		\
	gimpbrushmenu.h		\
	gimpfontmenu.h		\
	gimpgradientmenu.h	\
	gimppatternmenu.h

libgimp_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	$(no_undefined) \
	$(libgimp_export_symbols)

libgimp_1_3_la_LIBADD = $(libgimpcolor) $(libgimpbase) $(GLIB_LIBS) 

libgimp_1_3_la_DEPENDENCIES = $(gimp_def)

libgimpui_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	$(no_undefined) \
	$(libgimpui_export_symbols)

libgimpui_1_3_la_LIBADD = $(libgimp) $(libgimpwidgets) $(libgimpcolor) \
	 $(libgimpbase) $(libgimpmodule) $(GTK_LIBS)

libgimpui_1_3_la_DEPENDENCIES = $(gimpui_def) $(libgimp)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
