## Process this file with automake to produce Makefile.in

libgimpcolorincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpcolor

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpColor\"		\
	@GIMP_THREAD_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpcolor.def

lib_LTLIBRARIES = libgimpcolor-1.3.la

libgimpcolor_1_3_la_SOURCES = @STRIP_BEGIN@ \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.c	\
	gimpadaptivesupersample.h	\
	gimpbilinear.c			\
	gimpbilinear.h			\
	gimpcolorspace.c		\
	gimpcolorspace.h		\
	gimphsv.c			\
	gimphsv.h			\
	gimprgb.c			\
	gimprgb.h			\
@STRIP_END@

libgimpcolorinclude_HEADERS = @STRIP_BEGIN@ \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.h	\
	gimpbilinear.h			\
	gimpcolorspace.h		\
	gimphsv.h			\
	gimprgb.h			\
@STRIP_END@

EXTRA_HEADERS +=

libgimpcolor_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimpcolor_1_3_la_LIBADD = $(GLIB_LIBS) -lm

## this is probably cruft, but may not be.  -klm
## makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimpcolor/makefile.mingw.in
##	cd $(top_builddir) && CONFIG_FILES=libgimpcolor/$@ CONFIG_HEADERS= $(SHELL) ./config.status
##