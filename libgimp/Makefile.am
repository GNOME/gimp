## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/libgimp

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"	\
	-DDATADIR=\""$(gimpdatadir)"\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	gimpfeatures.h.in	\
	makefile.msc		\
	makefile.cygwin		\
	gimp.def		\
	gimpui.def

lib_LTLIBRARIES = libgimp.la libgimpui.la
noinst_LIBRARIES = libgimpi.a

PDB_WRAPPERS_C = \
	gimpbrushes_pdb.c		\
	gimpbrushselect_pdb.c		\
	gimpchannel_pdb.c		\
	gimpchannelops_pdb.c		\
	gimpcolor_pdb.c			\
	gimpconvert_pdb.c		\
	gimpdisplay_pdb.c		\
	gimpdrawable_pdb.c		\
	gimpedit_pdb.c			\
	gimpfileops_pdb.c		\
	gimpfloatingsel_pdb.c		\
	gimpgimprc_pdb.c		\
	gimpgradient_pdb.c		\
	gimpgradientselect_pdb.c	\
	gimpguides_pdb.c		\
	gimphelp_pdb.c			\
	gimpimage_pdb.c			\
	gimplayer_pdb.c			\
	gimpmessage_pdb.c		\
	gimppalette_pdb.c		\
	gimpparasite_pdb.c		\
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
	gimpchannelops_pdb.h		\
	gimpcolor_pdb.h			\
	gimpconvert_pdb.h		\
	gimpdisplay_pdb.h		\
	gimpdrawable_pdb.h		\
	gimpedit_pdb.h			\
	gimpfileops_pdb.h		\
	gimpfloatingsel_pdb.h		\
	gimpgimprc_pdb.h		\
	gimpgradient_pdb.h		\
	gimpgradientselect_pdb.h	\
	gimpguides_pdb.h		\
	gimphelp_pdb.h			\
	gimpimage_pdb.h			\
	gimplayer_pdb.h			\
	gimpmessage_pdb.h		\
	gimppalette_pdb.h		\
	gimpparasite_pdb.h		\
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
	gimpchainbutton.c	\
	gimpchainbutton.h	\
	gimpcolorbutton.c	\
	gimpcolorspace.c	\
	gimpcolorspace.h	\
	gimpdialog.c		\
	gimpdialog.h		\
	gimpfileselection.c	\
	gimpfileselection.h	\
	gimphelpui.c		\
	gimphelpui.h		\
	gimpmatrix.c		\
	gimpmatrix.h		\
	gimppatheditor.c	\
	gimppatheditor.h	\
	gimppixmap.c		\
	gimppixmap.h		\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpquerybox.c		\
	gimpquerybox.h		\
	gimpsignal.c            \
	gimpsignal.h            \
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpunitmenu.c		\
	gimpunitmenu.h		\
	gimputils.c		\
	gimputils.h		\
	gimpvector.c		\
	gimpvector.h		\
	gimpwidgets.c		\
	gimpwidgets.h		\
	gimpwire.c		\
	gimpwire.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h

if STATICLIBS
## Evil hack to insure all deps are satisfied on first-run make
libgimpi_a_DEPENDENCIES = libgimp.la
endif

# help `make' along by giving another name for the file, which it knows
# how to build
../libgimp/gimpfeatures.h: gimpfeatures.h
	@:

libgimp_la_SOURCES = \
	gimp.c			\
	gimp.h			\
	${PDB_WRAPPERS_C}	\
	${PDB_WRAPPERS_H}	\
	gimpcolorspace.c	\
	gimpcolorspace.h	\
	gimpdrawable.c		\
	gimpdrawable.h		\
	gimpenv.c		\
	gimpenv.h		\
	gimpgradientselect.c	\
	gimpgradientselect.h	\
	gimphelp.c		\
	gimpimage.c		\
	gimpimage.h		\
	gimpmatrix.c		\
	gimpmatrix.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	gimppixelrgn.c		\
	gimppixelrgn.h		\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpselection.c		\
	gimpselection.h		\
	gimpsignal.c		\
	gimpsignal.h		\
	gimptile.c		\
	gimptile.h		\
	gimpunit.c		\
	gimpunit.h		\
	gimputils.c		\
	gimputils.h		\
	gimpvector.c		\
	gimpvector.h		\
	gimpwire.c		\
	gimpwire.h		\
	stdplugins-intl.h	\
	libgimp-intl.h

libgimpui_la_SOURCES = \
	gimpmenu.c		\
	gimpbrushmenu.c		\
	gimpgradientmenu.c 	\
	gimppatternmenu.c	\
	gimpchainbutton.c	\
	gimpcolorbutton.c	\
	gimpcolorbutton.h	\
	gimpdialog.c		\
	gimpexport.c		\
	gimpfileselection.c	\
	gimphelpui.c		\
	gimppatheditor.c	\
	gimppixmap.c		\
	gimpquerybox.c		\
	gimpsizeentry.c		\
	gimpui.c		\
	gimpunitmenu.c		\
	gimpwidgets.c

gimpinclude_HEADERS = \
	gimp.h			\
	${PDB_WRAPPERS_H}	\
	gimpchainbutton.h	\
	gimpcolorbutton.h	\
	gimpcolordisplay.h	\
	gimpcolorselector.h	\
	gimpcolorspace.h	\
        gimpcompat.h		\
	gimpdrawable.h		\
	gimpdialog.h		\
	gimpenums.h		\
	gimpenv.h		\
	gimpexport.h		\
	gimpfeatures.h		\
	gimpfileselection.h	\
	gimpgradientselect.h	\
	gimphelpui.h		\
	gimpimage.h		\
	gimplimits.h		\
	gimpmath.h		\
	gimpmatrix.h		\
	gimpmenu.h		\
	gimpmodule.h		\
	gimpparasite.h		\
	gimpparasiteio.h	\
	gimppatheditor.h	\
	gimppixelrgn.h		\
	gimppixmap.h		\
	gimpquerybox.h		\
	gimpselection.h		\
	gimpsignal.h		\
	gimpsizeentry.h		\
	gimptile.h		\
	gimptypes.h		\
	gimpui.h		\
	gimpunit.h		\
	gimpunitmenu.h		\
	gimputils.h		\
	gimpvector.h		\
	gimpwidgets.h		\
	gimpintl.h


libgimp_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)
libgimp_la_LIBADD = $(GLIB_LIBS) -lm

libgimpui_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)
libgimpui_la_LIBADD = $(GTK_LIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
