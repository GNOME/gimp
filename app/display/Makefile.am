## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappdisplay.a

libappdisplay_a_SOURCES = @STRIP_BEGIN@ \
	display-types.h			\
	gimpdisplay.c			\
	gimpdisplay.h			\
	gimpdisplay-callbacks.c		\
	gimpdisplay-callbacks.h		\
	gimpdisplay-marching-ants.h	\
	gimpdisplay-ops.c		\
	gimpdisplay-ops.h		\
	gimpdisplay-render.c		\
	gimpdisplay-render.h		\
	gimpdisplay-scale.c		\
	gimpdisplay-scale.h		\
	gimpdisplay-scroll.c		\
	gimpdisplay-scroll.h		\
	gimpdisplay-selection.c		\
	gimpdisplay-selection.h		\
	gimpdisplayshell.c		\
	gimpdisplayshell.h		\
	\
	gximage.c			\
	gximage.h			\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Display\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
