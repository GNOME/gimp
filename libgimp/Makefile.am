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
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpunitmenu.c		\
	gimpunitmenu.h		\
	gimpvector.c		\
	gimpvector.h		\
	gimpwidgets.c		\
	gimpwidgets.h		\
	gimpwire.c		\
	gimpwire.h		\
	parasite.c		\
	parasite.h		\
	parasiteF.h		\
	parasiteP.h		\
	parasiteio.c		\
	parasiteio.h

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
	gimpchannel.c		\
	gimpcolorbutton.h	\
	gimpcolorspace.c	\
	gimpdisplay.c		\
	gimpdrawable.c		\
	gimpenv.c		\
	gimpgradient.c		\
	gimphelp.c		\
	gimpimage.c		\
	gimplayer.c		\
	gimpmatrix.c		\
	gimpmatrix.h		\
	gimppalette.c		\
	gimpparasite.c		\
	gimppixelrgn.c		\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpselection.c		\
	gimptile.c		\
	gimpunit.c		\
	gimpunit.h		\
	gimpvector.c		\
	gimpvector.h		\
	gimpwire.c		\
	gimpwire.h		\
	parasite.c		\
	parasite.h		\
	parasiteF.h		\
	parasiteP.h		\
	parasiteio.c		\
	parasiteio.h		\
	stdplugins-intl.h	\
	libgimp-intl.h

libgimpui_la_SOURCES = \
	gimpmenu.c		\
	gimpbrushmenu.c		\
	gimpgradientmenu.c 	\
	gimppatternmenu.c	\
	gimpchainbutton.c	\
	gimpcolorbutton.c	\
	gimpdialog.c		\
	gimpexport.c		\
	gimpfileselection.c	\
	gimphelpui.c		\
	gimppatheditor.c	\
	gimppixmap.c		\
	gimpquerybox.c		\
	gimpsizeentry.c		\
	gimpunitmenu.c		\
	gimpwidgets.c

gimpinclude_HEADERS = \
	color_display.h		\
	color_selector.h	\
	gimp.h			\
	gimpchainbutton.h	\
	gimpcolorbutton.h	\
	gimpcolorspace.h	\
        gimpcompat.h		\
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
	gimppatheditor.h	\
	gimppixmap.h		\
	gimpquerybox.h		\
	gimpsizeentry.h		\
	gimpui.h		\
	gimpunit.h              \
	gimpunitmenu.h          \
	gimpvector.h		\
	gimpwidgets.h		\
	gimpintl.h		\
	parasite.h		\
	parasiteF.h		\
	parasiteP.h		\
	parasiteio.h

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
