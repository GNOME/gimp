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
	gimpfileselection.c	\
	gimpfileselection.h	\
	gimpmatrix.c		\
	gimpmatrix.h		\
	gimppatheditor.c	\
	gimppatheditor.h	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpunitmenu.c		\
	gimpunitmenu.h		\
	gimpwire.c		\
	gimpwire.h		\
	gserialize.c		\
	gserialize.h		\
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
	gimpdisplay.c		\
	gimpdrawable.c		\
	gimpenv.c		\
	gimpgradient.c		\
	gimpimage.c		\
	gimplayer.c		\
	gimpmatrix.c		\
	gimpmatrix.h		\
	gimppalette.c		\
	gimpparasite.c		\
	gimppixelrgn.c		\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimptile.c		\
	gimpunit.c		\
	gimpunit.h		\
	gimpwire.c		\
	gimpwire.h		\
	gserialize.c		\
	gserialize.h		\
	parasite.c		\
	parasite.h		\
	parasiteF.h		\
	parasiteP.h		\
	parasiteio.c		\
	parasiteio.h		\
	stdplugins-intl.h

libgimpui_la_SOURCES = \
	gimpmenu.c		\
	gimpbrushmenu.c		\
	gimpgradientmenu.c 	\
	gimppatternmenu.c	\
	gimpexport.c		\
	gimpchainbutton.c	\
	gimpfileselection.c	\
	gimppatheditor.c	\
	gimpsizeentry.c		\
	gimpunitmenu.c

gimpinclude_HEADERS = \
	color_display.h		\
	color_selector.h	\
	gimp.h			\
	gimpchainbutton.h	\
        gimpcompat.h		\
	gimpenums.h		\
	gimpenv.h		\
	gimpexport.h		\
	gimpfeatures.h		\
	gimpfileselection.h	\
	gimplimits.h		\
	gimpmath.h		\
	gimpmatrix.h		\
	gimpmenu.h		\
	gimpmodule.h		\
	gimppatheditor.h	\
	gimpsizeentry.h		\
	gimpui.h		\
	gimpunit.h              \
	gimpunitmenu.h          \
	gimpintl.h		\
	gserialize.h		\
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
