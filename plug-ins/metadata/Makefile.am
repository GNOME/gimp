## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = metadata

metadata_SOURCES = \
	metadata.c	\
	interface.h	\
	interface.c	\
	xmp-model.h	\
	xmp-model.c	\
	xmp-parse.h	\
	xmp-parse.c	\
	xmp-encode.h	\
	xmp-encode.c	
#	exif-decode.h	\
#	exif-decode.c	\
#	exif-encode.h	\
#	exif-encode.c	\
#	iptc-decode.h	\
#	iptc-decode.c	

noinst_PROGRAMS = xmpdump
xmpdump_SOURCES = \
	xmpdump.c	\
	xmp-parse.h	\
	xmp-parse.c	

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(EXIF_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
