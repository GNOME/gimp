## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Vectors\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

noinst_LIBRARIES = libappvectors.a

libappvectors_a_sources = @STRIP_BEGIN@ \
	vectors-types.h			\
	gimpanchor.h			\
	gimpbezierstroke.h		\
	gimpbezierstroke.c		\
	gimpstroke.h			\
	gimpstroke.c			\
	gimpvectors.c			\
	gimpvectors.h			\
	gimpvectors-preview.c		\
	gimpvectors-preview.h		\
@STRIP_END@

libappvectors_a_SOURCES = $(libappvectors_a_sources)


