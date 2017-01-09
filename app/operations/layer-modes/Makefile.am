## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Layer-Modes\"	\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_builddir)/app			\
	-I$(top_srcdir)/app			\
	$(CAIRO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = \
	libapplayermodes-generic.a	\
	libapplayermodes-sse2.a		\
	libapplayermodes-sse4.a		\
	libapplayermodes.a

libapplayermodes_generic_a_sources = \
	gimpoperationnormal.c		\
	gimpoperationnormal.h		\
	gimpoperationdodge.c		\
	gimpoperationdodge.h		\
	gimpoperationmultiply.c		\
	gimpoperationmultiply.h		\
	gimpoperationoverlay.c		\
	gimpoperationoverlay.h		\
	gimpoperationscreen.c  		\
	gimpoperationscreen.h           \
	gimpoperationdifference.c	\
	gimpoperationdifference.h

libapplayermodes_sse2_a_sources = \
	gimpoperationnormal-sse2.c

libapplayermodes_sse4_a_sources = \
	gimpoperationnormal-sse4.c


libapplayermodes_generic_a_SOURCES = $(libapplayermodes_generic_a_sources)

libapplayermodes_sse2_a_SOURCES = $(libapplayermodes_sse2_a_sources)

libapplayermodes_sse2_a_CFLAGS = $(SSE2_EXTRA_CFLAGS)

libapplayermodes_sse4_a_SOURCES = $(libapplayermodes_sse4_a_sources)

libapplayermodes_sse4_a_CFLAGS = $(SSE4_1_EXTRA_CFLAGS)

libapplayermodes_a_SOURCES =


libapplayermodes.a: libapplayermodes-generic.a \
                    libapplayermodes-sse2.a \
                    libapplayermodes-sse4.a
	$(AR) $(ARFLAGS) libapplayermodes.a \
	  $(libapplayermodes_generic_a_OBJECTS) \
	  $(libapplayermodes_sse2_a_OBJECTS) \
	  $(libapplayermodes_sse4_a_OBJECTS)
	$(RANLIB) libapplayermodes.a
