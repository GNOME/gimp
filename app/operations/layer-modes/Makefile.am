## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Layer-Modes\"	\
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
	ligma-layer-modes.c			\
	ligma-layer-modes.h			\
	\
	ligmaoperationlayermode.c		\
	ligmaoperationlayermode.h		\
	ligmaoperationlayermode-blend.c		\
	ligmaoperationlayermode-blend.h		\
	ligmaoperationlayermode-composite.c	\
	ligmaoperationlayermode-composite.h	\
	\
	ligmaoperationantierase.c		\
	ligmaoperationantierase.h		\
	ligmaoperationbehind.c			\
	ligmaoperationbehind.h			\
	ligmaoperationdissolve.c			\
	ligmaoperationdissolve.h			\
	ligmaoperationerase.c			\
	ligmaoperationerase.h			\
	ligmaoperationmerge.c			\
	ligmaoperationmerge.h			\
	ligmaoperationnormal.c			\
	ligmaoperationnormal.h			\
	ligmaoperationpassthrough.c		\
	ligmaoperationpassthrough.h		\
	ligmaoperationreplace.c			\
	ligmaoperationreplace.h			\
	ligmaoperationsplit.c			\
	ligmaoperationsplit.h

libapplayermodes_sse2_a_sources = \
	ligmaoperationlayermode-composite-sse2.c	\
	\
	ligmaoperationnormal-sse2.c

libapplayermodes_sse4_a_sources = \
	ligmaoperationnormal-sse4.c


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
