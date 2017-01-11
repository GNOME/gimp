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
	gimpoperationdodgelegacy.h      \
	gimpoperationscreenlegacy.c	\
	gimpoperationscreenlegacy.h     \
	gimpoperationdifferencelegacy.c	\
	gimpoperationdifferencelegacy.h \
	gimpoperationadditionlegacy.c	\
	gimpoperationadditionlegacy.h   \
	gimpoperationsubtractlegacy.c	\
	gimpoperationsubtractlegacy.h   \
	gimpoperationdarkenonlylegacy.c \
	gimpoperationdarkenonlylegacy.h \
	gimpoperationlightenonlylegacy.c\
	gimpoperationlightenonlylegacy.h\
	gimpoperationhsvhuelegacy.c     \
	gimpoperationhsvhuelegacy.h     \
	gimpoperationhsvsaturationlegacy.c \
	gimpoperationhsvsaturationlegacy.h \
	gimpoperationhsvcolorlegacy.c   \
	gimpoperationhsvcolorlegacy.h   \
	gimpoperationhsvvaluelegacy.c   \
	gimpoperationhsvvaluelegacy.h   \
	gimpoperationdividelegacy.c     \
	gimpoperationdividelegacy.h     \
	gimpoperationburnlegacy.c   	\
	gimpoperationburnlegacy.h 
