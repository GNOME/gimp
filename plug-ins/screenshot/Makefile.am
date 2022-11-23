## Process this file with automake to produce Makefile.in

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
screenshot_RC = screenshot-win32-res.o
endif

AM_LDFLAGS = $(mwindows)

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(XFIXES_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(SCREENSHOT_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(screenshot_RC)

libexecdir = $(ligmaplugindir)/plug-ins/screenshot

libexec_PROGRAMS = screenshot

EXTRA_PROGRAMS = screenshot

screenshot_SOURCES = \
	screenshot.c				\
	screenshot.h				\
	screenshot-freedesktop.c		\
	screenshot-freedesktop.h		\
	screenshot-icon.h			\
	screenshot-osx.c			\
	screenshot-osx.h			\
	screenshot-x11.c			\
	screenshot-x11.h			\
	screenshot-win32.rc			\
	screenshot-win32.c			\
	screenshot-win32.h			\
	screenshot-win32-dwm-api.h		\
	screenshot-win32-magnification-api.h	\
	screenshot-win32-resource.h

EXTRA_DIST = \
	screenshot-win32-select.cur	\
	screenshot-win32-small.ico	\
	screenshot-win32.ico

if OS_WIN32
screenshot-win32-res.o: screenshot-win32.rc screenshot-win32-select.cur screenshot-win32-small.ico screenshot-win32.ico
	$(WINDRES) $(srcdir)/screenshot-win32.rc screenshot-win32-res.o
endif
