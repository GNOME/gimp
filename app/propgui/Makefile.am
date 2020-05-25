## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
        -DISO_CODES_LOCATION=\"$(ISO_CODES_LOCATION)\"		\
        -DISO_CODES_LOCALEDIR=\"$(ISO_CODES_LOCALEDIR)\"	\
	-DG_LOG_DOMAIN=\"Gimp-PropGUI\"				\
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
	gimppropgui.c				\
	gimppropgui.h				\
	gimppropgui-channel-mixer.c		\
	gimppropgui-channel-mixer.h		\
	gimppropgui-color-balance.c		\
	gimppropgui-color-balance.h		\
	gimppropgui-color-rotate.c		\
	gimppropgui-color-rotate.h		\
	gimppropgui-color-to-alpha.c		\
	gimppropgui-color-to-alpha.h		\
	gimppropgui-convolution-matrix.c	\
	gimppropgui-convolution-matrix.h	\
	gimppropgui-diffraction-patterns.c	\
	gimppropgui-diffraction-patterns.h	\
	gimppropgui-eval.c			\
	gimppropgui-eval.h			\
	gimppropgui-focus-blur.c		\
	gimppropgui-focus-blur.h		\
	gimppropgui-generic.c			\
	gimppropgui-generic.h			\
	gimppropgui-hue-saturation.c		\
	gimppropgui-hue-saturation.h		\
	gimppropgui-motion-blur-circular.c	\
	gimppropgui-motion-blur-circular.h	\
	gimppropgui-motion-blur-linear.c	\
	gimppropgui-motion-blur-linear.h	\
	gimppropgui-motion-blur-zoom.c		\
	gimppropgui-motion-blur-zoom.h		\
	gimppropgui-newsprint.c			\
	gimppropgui-newsprint.h			\
	gimppropgui-panorama-projection.c	\
	gimppropgui-panorama-projection.h	\
	gimppropgui-recursive-transform.c	\
	gimppropgui-recursive-transform.h	\
	gimppropgui-shadows-highlights.c	\
	gimppropgui-shadows-highlights.h	\
	gimppropgui-spiral.c			\
	gimppropgui-spiral.h			\
	gimppropgui-supernova.c			\
	gimppropgui-supernova.h			\
	gimppropgui-utils.c			\
	gimppropgui-utils.h			\
	gimppropgui-vignette.c			\
	gimppropgui-vignette.h
