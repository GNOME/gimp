## Process this file with automake to produce Makefile.in


## When updating gimp-print, do not forget to update 
## GIMP_PRINT_VERSION and GIMP_PRINT_RELEASE_DATE 
## in the toplevel configure.in !!


libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = escputil

print_SOURCES = @STRIP_BEGIN@ \
	print-canon.c		\
	print-dither.c		\
	print-escp2.c		\
	print-intl.h		\
	print-pcl.c		\
	print-ps.c		\
	print-util.c		\
	print-weave.c		\
	print.h			\
	quickmatrix257.h	\
	ran.367.179.h		\
	print-image-gimp.c	\
	print.c			\
	gimp_color_window.c	\
	gimp_main_window.c	\
	print_gimp.h		\
@STRIP_END@

escputil_SOURCES = escputil.c

man_MANS = escputil.1

EXTRA_DIST = print-printers.c

AM_CPPFLAGS = @STRIP_BEGIN@ \
        -DLOCALEDIR=\""$(localedir)"\"			\
	-DVERSION=\"$(GIMP_PRINT_VERSION)\"		\
	-DRELEASE_DATE=\"$(GIMP_PRINT_RELEASE_DATE)\"	\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)							\
@STRIP_END@

escputil_LDADD =
