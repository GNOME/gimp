## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappdisplay.a

libappdisplay_a_SOURCES = @STRIP_BEGIN@ 	\
	display-types.h				\
	gimpdisplay.c				\
	gimpdisplay.h				\
	gimpdisplay-area.c			\
	gimpdisplay-area.h			\
	gimpdisplay-foreach.c			\
	gimpdisplay-foreach.h			\
	gimpdisplay-handlers.c			\
	gimpdisplay-handlers.h			\
	gimpdisplayshell.c			\
	gimpdisplayshell.h			\
	gimpdisplayshell-callbacks.c		\
	gimpdisplayshell-callbacks.h		\
	gimpdisplayshell-dnd.c			\
	gimpdisplayshell-dnd.h			\
	gimpdisplayshell-handlers.c		\
	gimpdisplayshell-handlers.h		\
	gimpdisplayshell-layer-select.c		\
	gimpdisplayshell-layer-select.h		\
	gimpdisplayshell-marching-ants.h	\
	gimpdisplayshell-qmask.c		\
	gimpdisplayshell-qmask.h		\
	gimpdisplayshell-render.c		\
	gimpdisplayshell-render.h		\
	gimpdisplayshell-scale.c		\
	gimpdisplayshell-scale.h		\
	gimpdisplayshell-scroll.c		\
	gimpdisplayshell-scroll.h		\
	gimpdisplayshell-selection.c		\
	gimpdisplayshell-selection.h		\
	\
	gximage.c				\
	gximage.h				\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Display\"		\
	@GIMP_THREAD_FLAGS@ 			\
	@GIMP_MP_FLAGS@				\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/app			\
	$(GTK_CFLAGS)				\
	-I$(includedir)				\
@STRIP_END@
