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
	gimp_pdb.h		\
	gimpchannel_pdb.h	\
	gimpchannel_pdb.c	\
	gimpcolorspace.c	\
	gimpcolorspace.h	\
	gimpdisplay_pdb.c	\
	gimpdisplay_pdb.h	\
	gimpdrawable.c		\
	gimpdrawable.h		\
	gimpdrawable_pdb.c	\
	gimpdrawable_pdb.h	\
	gimpenv.c		\
	gimpenv.h		\
	gimpfileops_pdb.c	\
	gimpfileops_pdb.h	\
	gimpgradient_pdb.c	\
	gimpguides_pdb.c	\
	gimphelp.c		\
	gimphelp_pdb.c		\
	gimphelp_pdb.h		\
	gimpimage_pdb.c		\
	gimpimage_pdb.h		\
	gimplayer_pdb.c		\
	gimplayer_pdb.h		\
	gimpmatrix.c		\
	gimpmatrix.h		\
	gimpmessage_pdb.c	\
	gimpmessage_pdb.h	\
	gimppalette_pdb.c	\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	gimpparasite_pdb.c	\
	gimpparasite_pdb.h	\
	gimppixelrgn.c		\
	gimppixelrgn.h		\
	gimpplugin_pdb.c	\
	gimpplugin_pdb.h	\
	gimpproceduraldb_pdb.c	\
	gimpproceduraldb_pdb.h	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpselection_pdb.c	\
	gimpselection_pdb.h	\
	gimpsignal.c		\
	gimpsignal.h		\
	gimptile.c		\
	gimptile.h		\
	gimpunit_pdb.c		\
	gimpunit_pdb.h		\
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
	gimp_pdb.h		\
	gimpchannel_pdb.h	\
	gimpdisplay_pdb.h	\
	gimpdrawable_pdb.h	\
	gimpfileops_pdb.h	\
	gimpgradient_pdb.h	\
	gimphelp_pdb.h		\
	gimpimage_pdb.h		\
	gimplayer_pdb.h		\
	gimpmessage_pdb.h	\
	gimppalette_pdb.h	\
	gimpparasite_pdb.h	\
	gimpplugin_pdb.h	\
	gimpproceduraldb_pdb.h	\
	gimpselection_pdb.h	\
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
	gimphelpui.h		\
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
