## Process this file with automake to produce Makefile.in


## When updating gimp-print, do not forget to update 
## GIMP_PRINT_VERSION and GIMP_PRINT_RELEASE_DATE 
## in the toplevel configure.in !!


libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = print-printers.c

libexec_PROGRAMS = print

bin_PROGRAMS = escputil

print_SOURCES = \
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
	print_gimp.h

escputil_SOURCES = escputil.c

man_MANS = escputil.1

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"		\
	@LP_DEF@				\
	@LPSTAT_DEF@				\
	@LPR_DEF@				\
	@LPC_DEF@				\
	-DVERSION=\"$(GIMP_PRINT_VERSION)\"	\
	-DRELEASE_DATE=\"$(GIMP_PRINT_RELEASE_DATE)\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

escputil_LDADD =

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
