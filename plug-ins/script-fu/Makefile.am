## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = siod scripts

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = script-fu

script_fu_SOURCES = \
	script-fu.c			\
	script-fu-console.c		\
	script-fu-console.h		\
	script-fu-text-console.h	\
	script-fu-text-console.c	\
	script-fu-constants.c		\
	script-fu-constants.h		\
	script-fu-intl.h		\
	script-fu-scripts.c		\
	script-fu-scripts.h		\
	script-fu-server.c		\
	script-fu-server.h		\
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
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o		\
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	siod/libsiod.a							\
	$(GTK_LIBS)							\
	$(SOCKET_LIBS)							\
	$(RT_LIBS)							\
	$(INTLLIBS)							\
	$(REGEXREPL)
