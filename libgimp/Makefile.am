## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/libgimp

CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = gimpfeatures.h.in

lib_LTLIBRARIES = libgimp.la libgimpui.la
noinst_LIBRARIES = libgimpi.a

libgimpi_a_SOURCES = \
	gimpmatrix.c	\
	gimpmatrix.h	\
	gimpprotocol.c	\
	gimpprotocol.h	\
	gimpsizeentry.c	\
	gimpsizeentry.h	\
	gimpunit.c      \
	gimpunit.h      \
	gimpunitmenu.c  \
	gimpunitmenu.h  \
	gimpwire.c	\
	gimpwire.h	\
	gserialize.c	\
	gserialize.h	\
	parasite.c	\
	parasite.h	\
	parasiteF.h	\
	parasiteP.h

if STATICLIBS
## Evil hack to insure all deps are satisfied on first-run make
libgimpi_a_DEPENDENCIES = libgimp.la
endif

libgimp_la_SOURCES = \
	gimp.c		\
	gimpchannel.c	\
	gimpdisplay.c	\
	gimpdrawable.c	\
	gimpgradient.c	\
	gimpimage.c	\
	gimplayer.c	\
	gimpmatrix.c	\
	gimpmatrix.h	\
	gimppalette.c	\
	gimpparasite.c	\
	gimppixelrgn.c	\
	gimpprotocol.c	\
	gimpprotocol.h	\
	gimptile.c	\
	gimpwire.c	\
	gimpwire.h	\
	gserialize.c	\
	gserialize.h	\
	parasite.c	\
	parasite.h	\
	parasiteF.h	\
	parasiteP.h

libgimpui_la_SOURCES = \
	gimpmenu.c		\
	gimpbrushmenu.c		\
	gimpgradientmenu.c 	\
	gimppatternmenu.c

gimpinclude_HEADERS = \
	color_selector.h	\
	gimp.h			\
	gimpenums.h		\
	gimpfeatures.h		\
	gimpmatrix.h		\
	gimpmenu.h		\
	gimpmodule.h		\
	gimpsizeentry.h		\
	gimpui.h		\
	gimpunit.h              \
	gimpunitmenu.h          \
	gimpintl.h		\
	gserialize.h		\
	parasite.h		\
	parasiteF.h		\
	parasiteP.h

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
