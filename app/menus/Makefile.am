## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappmenus.a

libappmenus_a_SOURCES = \
	menus-types.h		\
	menus.c			\
	menus.h			\
	file-menu.c		\
	file-menu.h		\
	image-menu.c		\
	image-menu.h		\
	plug-in-menus.c		\
	plug-in-menus.h		\
	tool-options-menu.c	\
	tool-options-menu.h	\
	toolbox-menu.c		\
	toolbox-menu.h


EXTRA_DIST = \
	makefile.msc

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Menus\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	-I$(includedir)
