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
	gimp-layer-modes.c			\
	gimp-layer-modes.h			\
	\
	gimpoperationlayermode.c		\
	gimpoperationlayermode.h		\
	gimpoperationlayermode-blend.c		\
	gimpoperationlayermode-blend.h		\
	gimpoperationlayermode-composite.c	\
	gimpoperationlayermode-composite.h	\
	\
	gimpoperationantierase.c		\
	gimpoperationantierase.h		\
	gimpoperationbehind.c			\
	gimpoperationbehind.h			\
	gimpoperationdissolve.c			\
	gimpoperationdissolve.h			\
	gimpoperationerase.c			\
	gimpoperationerase.h			\
	gimpoperationmerge.c			\
	gimpoperationmerge.h			\
	gimpoperationnormal.c			\
	gimpoperationnormal.h			\
	gimpoperationpassthrough.c		\
	gimpoperationpassthrough.h		\
	gimpoperationreplace.c			\
	gimpoperationreplace.h			\
	gimpoperationsplit.c			\
	gimpoperationsplit.h

libapplayermodes_sse2_a_sources = \
	gimpoperationlayermode-composite-sse2.c	\
	\
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
