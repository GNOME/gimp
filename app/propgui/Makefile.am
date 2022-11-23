## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
        -DISO_CODES_LOCATION=\"$(ISO_CODES_LOCATION)\"		\
        -DISO_CODES_LOCALEDIR=\"$(ISO_CODES_LOCALEDIR)\"	\
	-DG_LOG_DOMAIN=\"Ligma-PropGUI\"				\
	-I$(top_builddir)					\
	-I$(top_srcdir)						\
	-I$(top_builddir)/app					\
	-I$(top_srcdir)/app					\
	$(GEGL_CFLAGS)						\
	$(GTK_CFLAGS)						\
	-I$(includedir)

noinst_LIBRARIES = libapppropgui.a

libapppropgui_a_SOURCES = \
	propgui-types.h				\
	\
	ligmapropgui.c				\
	ligmapropgui.h				\
	ligmapropgui-channel-mixer.c		\
	ligmapropgui-channel-mixer.h		\
	ligmapropgui-color-balance.c		\
	ligmapropgui-color-balance.h		\
	ligmapropgui-color-rotate.c		\
	ligmapropgui-color-rotate.h		\
	ligmapropgui-color-to-alpha.c		\
	ligmapropgui-color-to-alpha.h		\
	ligmapropgui-convolution-matrix.c	\
	ligmapropgui-convolution-matrix.h	\
	ligmapropgui-diffraction-patterns.c	\
	ligmapropgui-diffraction-patterns.h	\
	ligmapropgui-eval.c			\
	ligmapropgui-eval.h			\
	ligmapropgui-focus-blur.c		\
	ligmapropgui-focus-blur.h		\
	ligmapropgui-generic.c			\
	ligmapropgui-generic.h			\
	ligmapropgui-hue-saturation.c		\
	ligmapropgui-hue-saturation.h		\
	ligmapropgui-motion-blur-circular.c	\
	ligmapropgui-motion-blur-circular.h	\
	ligmapropgui-motion-blur-linear.c	\
	ligmapropgui-motion-blur-linear.h	\
	ligmapropgui-motion-blur-zoom.c		\
	ligmapropgui-motion-blur-zoom.h		\
	ligmapropgui-newsprint.c			\
	ligmapropgui-newsprint.h			\
	ligmapropgui-panorama-projection.c	\
	ligmapropgui-panorama-projection.h	\
	ligmapropgui-recursive-transform.c	\
	ligmapropgui-recursive-transform.h	\
	ligmapropgui-shadows-highlights.c	\
	ligmapropgui-shadows-highlights.h	\
	ligmapropgui-spiral.c			\
	ligmapropgui-spiral.h			\
	ligmapropgui-supernova.c			\
	ligmapropgui-supernova.h			\
	ligmapropgui-utils.c			\
	ligmapropgui-utils.h			\
	ligmapropgui-vignette.c			\
	ligmapropgui-vignette.h
