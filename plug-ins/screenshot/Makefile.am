## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

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
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(SCREENSHOT_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(screenshot_RC)

libexecdir = $(gimpplugindir)/plug-ins/screenshot

libexec_PROGRAMS = screenshot

EXTRA_PROGRAMS = screenshot

screenshot_SOURCES = \
	screenshot.c			\
	screenshot.h			\
	screenshot-freedesktop.c	\
	screenshot-freedesktop.h	\
	screenshot-gnome-shell.c	\
	screenshot-gnome-shell.h	\
	screenshot-icon.h		\
	screenshot-kwin.c		\
	screenshot-kwin.h		\
	screenshot-osx.c		\
	screenshot-osx.h		\
	screenshot-x11.c		\
	screenshot-x11.h		\
	screenshot-win32.rc		\
	screenshot-win32.c		\
	screenshot-win32.h		\
	screenshot-win32-resource.h

EXTRA_DIST = \
	screenshot-win32-select.cur	\
	screenshot-win32-small.ico	\
	screenshot-win32.ico

if OS_WIN32
screenshot-win32-res.o: screenshot-win32.rc screenshot-win32-select.cur screenshot-win32-small.ico screenshot-win32.ico
	$(WINDRES) $(srcdir)/screenshot-win32.rc screenshot-win32-res.o
endif
