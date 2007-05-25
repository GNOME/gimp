## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

libtinyscheme=tinyscheme/libtinyscheme.a
libftx=ftx/libftx.a
libre=re/libre.a

if OS_WIN32
mwindows = -mwindows
WINSOCK_LIBS = -lws2_32
endif

AM_CFLAGS = \
	-DSTANDALONE=0     \
	-DUSE_INTERFACE=1  \
	-DUSE_STRLWR=0     \
	-I$(top_srcdir)    \
	$(GTK_CFLAGS)      \
	-I$(includedir)

AM_LDFLAGS = $(mwindows)

SUBDIRS = tinyscheme ftx re scripts


libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = script-fu

script_fu_SOURCES = \
	script-fu.c			\
	script-fu-console.c		\
	script-fu-console.h		\
	script-fu-interface.c		\
	script-fu-interface.h		\
	script-fu-text-console.h	\
	script-fu-text-console.c	\
	script-fu-intl.h		\
	script-fu-scripts.c		\
	script-fu-scripts.h		\
	script-fu-server.c		\
	script-fu-server.h		\
	script-fu-types.h		\
	script-fu-enums.h		\
	scheme-wrapper.c		\
	scheme-wrapper.h

if HAVE_GLIBC_REGEX
REGEXREPL =
else
REGEXREPL = $(top_builddir)/regexrepl/libregex.a
endif

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(libtinyscheme)	\
	$(libftx)		\
	$(libre)		\
	$(REGEXREPL)		\
	$(GTK_LIBS)		\
	$(SOCKET_LIBS)		\
	$(WINSOCK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
