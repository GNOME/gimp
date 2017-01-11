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
	gimpoperationpointlayermode.c	\
	gimpoperationpointlayermode.h	\
	gimplayermodefunctions.c	\
	gimplayermodefunctions.h	\
	\
	gimpoperationaddition.c		\
	gimpoperationaddition.h		\
	gimpoperationantierase.c	\
	gimpoperationantierase.h	\
	gimpoperationbehind.c		\
	gimpoperationbehind.h		\
	gimpoperationburn.c		\
	gimpoperationburn.h		\
	gimpoperationcolorerase.c	\
	gimpoperationcolorerase.h	\
	gimpoperationdarkenonly.c	\
	gimpoperationdarkenonly.h	\
	gimpoperationdifference.c	\
	gimpoperationdifference.h	\
	gimpoperationdissolve.c		\
	gimpoperationdissolve.h		\
	gimpoperationdivide.c		\
	gimpoperationdivide.h		\
	gimpoperationdodge.c		\
	gimpoperationdodge.h		\
	gimpoperationerase.c		\
	gimpoperationerase.h		\
	gimpoperationgrainextract.c	\
	gimpoperationgrainextract.h	\
	gimpoperationgrainmerge.c	\
	gimpoperationgrainmerge.h	\
	gimpoperationhardlight.c	\
	gimpoperationhardlight.h	\
	gimpoperationhsvcolor.c		\
	gimpoperationhsvcolor.h		\
	gimpoperationhsvhue.c		\
	gimpoperationhsvhue.h		\
	gimpoperationhsvsaturation.c	\
	gimpoperationhsvsaturation.h	\
	gimpoperationhsvvalue.c		\
	gimpoperationhsvvalue.h		\
	gimpoperationhsvvalue.h		\
	gimpoperationlchchroma.c	\
	gimpoperationlchchroma.h	\
	gimpoperationlchcolor.c		\
	gimpoperationlchcolor.h		\
	gimpoperationlchhue.c		\
	gimpoperationlchhue.h		\
	gimpoperationlchlightness.c	\
	gimpoperationlchlightness.h	\
	gimpoperationlightenonly.c	\
	gimpoperationlightenonly.h	\
	gimpoperationmultiply.c		\
	gimpoperationmultiply.h		\
	gimpoperationnormal.c		\
	gimpoperationnormal.h		\
	gimpoperationoverlay.c		\
	gimpoperationoverlay.h		\
	gimpoperationreplace.c		\
	gimpoperationreplace.h		\
	gimpoperationscreen.c		\
	gimpoperationscreen.h		\
	gimpoperationsoftlight.c	\
	gimpoperationsoftlight.h	\
	gimpoperationsubtract.c		\
	gimpoperationsubtract.h

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
