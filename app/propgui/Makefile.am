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
	gimppropgui-convolution-matrix.c	\
	gimppropgui-convolution-matrix.h	\
	gimppropgui-diffration-patterns.c	\
	gimppropgui-diffration-patterns.h	\
	gimppropgui-eval.c			\
	gimppropgui-eval.h			\
	gimppropgui-generic.c			\
	gimppropgui-generic.h			\
	gimppropgui-hue-saturation.c		\
	gimppropgui-hue-saturation.h		\
	gimppropgui-spiral.c			\
	gimppropgui-spiral.h			\
	gimppropgui-supernova.c			\
	gimppropgui-supernova.h			\
	gimppropgui-utils.c			\
	gimppropgui-utils.h
