## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimp

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"	\
	-DDATADIR=\""$(gimpdatadir)"\"	\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimp\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	gimpfeatures.h.in	\
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimp.def		\
	gimpui.def

lib_LTLIBRARIES = libgimp-1.3.la libgimpui-1.3.la
noinst_LIBRARIES = libgimpi.a

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
	gimppalette_pdb.c		\
	gimpparasite_pdb.c		\
	gimppaths_pdb.c			\
	gimppatterns_pdb.c		\
	gimppatternselect_pdb.c		\
	gimpplugin_pdb.c		\
	gimpproceduraldb_pdb.c		\
	gimpselection_pdb.c		\
	gimptexttool_pdb.c		\
	gimptools_pdb.c			\
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
	gimppalette_pdb.h		\
	gimpparasite_pdb.h		\
	gimppaths_pdb.h			\
	gimppatterns_pdb.h		\
	gimppatternselect_pdb.h		\
	gimpplugin_pdb.h		\
	gimpproceduraldb_pdb.h		\
	gimpselection_pdb.h		\
	gimptexttool_pdb.h		\
	gimptools_pdb.h			\
	gimpundo_pdb.h			\
	gimpunit_pdb.h

libgimpi_a_SOURCES = \
	gimpenv.c		\
	gimpenv.h		\
	gimpsignal.c            \
	gimpsignal.h            \
	gimputils.c		\
	gimputils.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpwire.c		\
	gimpwire.h

if STATICLIBS
## Evil hack to insure all deps are satisfied on first-run make
libgimpi_a_DEPENDENCIES = libgimp-1.3.la
endif

# help `make' along by giving another name for the file, which it knows
# how to build
../libgimp/gimpfeatures.h: gimpfeatures.h
	@:

libgimp_1_3_la_SOURCES = \
	gimp.c				\
	gimp.h				\
	gimptypes.h			\
	gimplimits.h			\
	${PDB_WRAPPERS_C}		\
	${PDB_WRAPPERS_H}		\
	gimpchannel.c			\
	gimpchannel.h			\
	gimpdrawable.c			\
	gimpdrawable.h			\
	gimpgradientselect.c		\
	gimpgradientselect.h		\
	gimphelp.c			\
	gimpimage.c			\
	gimpimage.h			\
	gimplayer.c			\
	gimplayer.h			\
	gimpparasite.c			\
	gimpparasite.h			\
	gimpparasiteio.c		\
	gimpparasiteio.h		\
	gimppixelrgn.c			\
	gimppixelrgn.h			\
	gimpproceduraldb.c		\
	gimpproceduraldb.h		\
	gimpselection.c			\
	gimpselection.h			\
	gimptile.c			\
	gimptile.h			\
	gimpunit.c			\
	gimpunit.h			\
	\
	gimpenv.c			\
	gimpenv.h			\
	gimpsignal.c			\
	gimpsignal.h			\
	gimputils.c			\
	gimputils.h			\
	\
	gimpprotocol.c			\
	gimpprotocol.h			\
	gimpwire.c			\
	gimpwire.h			\
	\
	gimpintl.h			\
	stdplugins-intl.h		\
	libgimp-intl.h

libgimpui_1_3_la_SOURCES = \
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
	gimpexport.h

gimpinclude_HEADERS = \
	gimpcolordisplay.h		\
	gimpcolorselector.h		\
	\
	gimp.h				\
	gimptypes.h			\
	${PDB_WRAPPERS_H}		\
	gimpchannel.h			\
	gimpdrawable.h			\
	gimpenums.h			\
	gimpenv.h			\
	gimpfeatures.h			\
	gimpgradientselect.h		\
	gimpimage.h			\
	gimplayer.h			\
	gimplimits.h			\
	gimpmodule.h			\
	gimpparasite.h			\
	gimpparasiteio.h		\
	gimppixelrgn.h			\
	gimpproceduraldb.h		\
	gimpselection.h			\
	gimpsignal.h			\
	gimptile.h			\
	gimpunit.h			\
	gimputils.h			\
	\
	gimpexport.h			\
	\
	gimpintl.h			\
	\
	gimpui.h			\
	gimpuitypes.h			\
	\
	gimpmenu.h

libgimp_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libgimp_1_3_la_LIBADD = $(GLIB_LIBS)

libgimpui_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libgimpui_1_3_la_LIBADD = $(GTK_LIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimp/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=libgimp/$@ CONFIG_HEADERS= $(SHELL) ./config.status
