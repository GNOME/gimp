## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgui.a

libappgui_a_SOURCES = \
	color-history.c		\
	color-history.h		\
	gui.c			\
	gui.h			\
	gui-vtable.c		\
	gui-vtable.h		\
	gui-types.h		\
	session.c		\
	session.h		\
	splash.c		\
	splash.h		\
	themes.c		\
	themes.h

EXTRA_DIST = makefile.msc

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	-I$(includedir)
