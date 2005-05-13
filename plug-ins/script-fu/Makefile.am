## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpprocbrowser = $(top_builddir)/plug-ins/dbbrowser/libgimpprocbrowser.la

if OS_WIN32
mwindows = -mwindows
WINSOCK_LIBS = -lws2_32
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = siod scripts

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
	siod-wrapper.c			\
	siod-wrapper.h

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

if HAVE_GLIBC_REGEX
REGEXREPL =
else
REGEXREPL = $(top_builddir)/regexrepl/libregex.a
endif

LDADD = \
	$(libgimpprocbrowser)	\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	siod/libsiod.a		\
	$(REGEXREPL)		\
	$(GTK_LIBS)		\
	$(SOCKET_LIBS)		\
	$(WINSOCK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
