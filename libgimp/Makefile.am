## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimp

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimp\"		\
	@GIMP_THREAD_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimp.def		\
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

libgimp_1_3_la_SOURCES = @STRIP_BEGIN@ \
	gimp.c				\
	gimp.h				\
	gimpcompat.h			\
	gimptypes.h			\
	${PDB_WRAPPERS_C}		\
	${PDB_WRAPPERS_H}		\
	gimpchannel.c			\
	gimpchannel.h			\
	gimpdrawable.c			\
	gimpdrawable.h			\
	gimphelp.c			\
	gimpimage.c			\
	gimpimage.h			\
	gimplayer.c			\
	gimplayer.h			\
	gimppixelrgn.c			\
	gimppixelrgn.h			\
	gimpproceduraldb.c		\
	gimpproceduraldb.h		\
	gimpselection.c			\
	gimpselection.h			\
	gimptile.c			\
	gimptile.h			\
	gimpunit.c			\
	\
	gimpintl.h			\
	stdplugins-intl.h		\
	libgimp-intl.h			\
@STRIP_END@

libgimpui_1_3_la_SOURCES = @STRIP_BEGIN@ \
	gimpui.c		\
	gimpui.h		\
	gimpuitypes.h		\
	\
	gimpmenu.c		\
	gimpmenu.h		\
	gimpbrushmenu.c		\
	gimpgradientmenu.c 	\
	gimppatternmenu.c	\
	\
	gimpexport.c		\
	gimpexport.h		\
@STRIP_END@

gimpinclude_HEADERS = @STRIP_BEGIN@ \
	gimpcolordisplay.h		\
	gimpcolorselector.h		\
	\
	gimp.h				\
	gimptypes.h			\
	${PDB_WRAPPERS_H}		\
	gimpchannel.h			\
	gimpdrawable.h			\
	gimpenums.h			\
	gimpimage.h			\
	gimplayer.h			\
	gimpmodule.h			\
	gimppixelrgn.h			\
	gimpproceduraldb.h		\
	gimpselection.h			\
	gimptile.h			\
	\
	gimpexport.h			\
	\
	gimpintl.h			\
	\
	gimpui.h			\
	gimpuitypes.h			\
	\
	gimpmenu.h			\
@STRIP_END@

EXTRA_HEADERS =

libgimp_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimp_1_3_la_LIBADD = $(GLIB_LIBS)

libgimpui_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimpui_1_3_la_LIBADD = $(GTK_LIBS)
