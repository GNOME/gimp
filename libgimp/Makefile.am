## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/libgimp

scriptdata =

EXTRA_DIST = gimpfeatures.h.in

lib_LTLIBRARIES = libgimp.la libgimpui.la
noinst_LIBRARIES = libgimpi.a

libgimpi_a_SOURCES = \
	gimpprotocol.c	\
	gimpprotocol.h	\
	gimpwire.c	\
	gimpwire.h

## Evil hack to insure all deps are satisfied on first-run make
libgimpi_a_DEPENDENCIES = libgimp.la

libgimp_la_SOURCES = \
	gimp.c		\
	gimpchannel.c	\
	gimpdisplay.c	\
	gimpdrawable.c	\
	gimpgradient.c	\
	gimpimage.c	\
	gimplayer.c	\
	gimppalette.c	\
	gimppixelrgn.c	\
	gimpprotocol.c	\
	gimpprotocol.h	\
	gimptile.c	\
	gimpwire.c	\
	gimpwire.h

libgimpui_la_SOURCES = \
	gimpmenu.c

gimpinclude_HEADERS = \
	gimp.h		\
	gimpenums.h	\
	gimpfeatures.h	\
	gimpmenu.h	\
	gimpui.h	\
	gimpintl.h

libgimp_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE)
libgimpui_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE)

CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
