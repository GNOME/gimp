## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Layer-Modes-Legacy\"	\
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_builddir)/app				\
	-I$(top_srcdir)/app				\
	$(CAIRO_CFLAGS)					\
	$(GEGL_CFLAGS)					\
	$(GDK_PIXBUF_CFLAGS)				\
	-I$(includedir)

noinst_LIBRARIES = \
	libapplayermodeslegacy.a

libapplayermodeslegacy_a_SOURCES = \
	gimpoperationmultiplylegacy.c	\
	gimpoperationmultiplylegacy.h	\
	gimpoperationdodgelegacy.c	\
	gimpoperationdodgelegacy.h
