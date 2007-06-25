## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GUI\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappgui.a

libappgui_a_SOURCES = \
	color-history.c		\
	color-history.h		\
	gui.c			\
	gui.h			\
	gui-message.c		\
	gui-message.h		\
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
