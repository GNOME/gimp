## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapppaint.a

libapppaint_a_SOURCES = @STRIP_BEGIN@ \
	paint-types.h			\
	paint.c				\
	paint.h				\
	gimpairbrush.c			\
	gimpairbrush.h			\
	gimpclone.c			\
	gimpclone.h			\
	gimpconvolve.c			\
	gimpconvolve.h			\
	gimpdodgeburn.c			\
	gimpdodgeburn.h			\
	gimperaser.c			\
	gimperaser.h			\
	gimppaintcore.c			\
	gimppaintcore.h			\
	gimppaintcore-kernels.h		\
	gimppaintcore-stroke.c		\
	gimppaintcore-stroke.h		\
	gimppaintoptions.c		\
	gimppaintoptions.h		\
	gimppencil.c			\
	gimppencil.h			\
	gimppaintbrush.c		\
	gimppaintbrush.h		\
	gimpsmudge.c			\
	gimpsmudge.h			\
@STRIP_END@

## This is a truly ugly hack
libapppaint_a_LIBADD = ../core/gimppaintinfo.o

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Paint\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	$(PANGOFT2_CFLAGS)		\
	-I$(includedir)			\
@STRIP_END@
