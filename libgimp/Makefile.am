## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/libgimp

scriptdata =

lib_LTLIBRARIES = libgimp-1.1.la libgimpui-1.1.la
noinst_LIBRARIES = libgimpi.a

libgimpi_a_SOURCES = \
	gimpprotocol.c	\
	gimpprotocol.h	\
	gimpwire.c	\
	gimpwire.h

## Evil hack to insure all deps are satisfied on first-run make
libgimpi_a_DEPENDENCIES = libgimp-1.1.la

libgimp_1_1_la_SOURCES = \
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

libgimpui_1_1_la_SOURCES = \
	gimpmenu.c

gimpinclude_HEADERS = \
	gimp.h		\
	gimpenums.h	\
	gimpmenu.h	\
	gimpui.h

libgimp_1_1_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libgimpui_1_1_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

libgimpui_1_1_la_INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
