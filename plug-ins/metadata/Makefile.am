## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = metadata

metadata_SOURCES = \
	metadata.c	\
	metadata.h	\
	base64.h	\
	base64.c	\
	xmp-model.h	\
	xmp-model.c	\
	xmp-parse.h	\
	xmp-parse.c	\
	xmp-encode.h	\
	xmp-encode.c	\
	xmp-schemas.h	\
	xmp-schemas.c	\
	interface.h	\
	interface.c	\
	exif-decode.h	\
	exif-decode.c	
#	exif-encode.h	\
#	exif-encode.c	\
#	iptc-decode.h	\
#	iptc-decode.c	

noinst_PROGRAMS = xmpdump
xmpdump_SOURCES = \
	xmpdump.c	\
	base64.h	\
	base64.c	\
	xmp-schemas.h	\
	xmp-schemas.c	\
	xmp-encode.h	\
	xmp-encode.c	\
	xmp-model.h	\
	xmp-model.c	\
	xmp-parse.h	\
	xmp-parse.c	

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(EXIF_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimp)		\
	$(libgimpui)		\
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(libgimpmath)		\
	$(EXIF_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)

# test program, not built by default

TESTS = testbase64$(EXEEXT)

EXTRA_PROGRAMS = testbase64

testbase64_SOURCES = \
	base64.h	\
	base64.c	\
	testbase64.c	

CLEANFILES = $(EXTRA_PROGRAMS)
