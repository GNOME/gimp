## Process this file with automake to produce Makefile.in

gimpincludedir = $(includedir)/libgimp

scriptdata =

EXTRA_DIST = gimpfeatures.h.in

lib_LTLIBRARIES = libgimp.la libgimpui.la
noinst_LIBRARIES = libgimpi.a

libgimpi_a_SOURCES =	gimpprotocol.c	\
			gimpprotocol.h	\
			gimpwire.c	\
			gimpwire.h

if STATICLIBS
libgimpi_a_DEPENDENCIES = libgimp.la
endif

libgimp_la_SOURCES =	gimp.c		\
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

libgimpui_la_SOURCES =	gimpmenu.c

gimpinclude_HEADERS =	gimp.h		\
			gimpenums.h	\
			gimpfeatures.h	\
			gimpmenu.h	\
			gimpui.h

libgimp_la_LDFLAGS = -version-info $(GIMP_MAJOR_VERSION):$(GIMP_MICRO_VERSION):0
libgimpui_la_LDFLAGS = -version-info $(GIMP_MAJOR_VERSION):$(GIMP_MICRO_VERSION):0

CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
