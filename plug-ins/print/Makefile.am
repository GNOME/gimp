## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = print-printers.c

libexec_PROGRAMS = print

print_SOURCES = \
	print-canon.c		\
	print-dither.c		\
	print-escp2.c		\
	print-pcl.c		\
	print-ps.c		\
	print-util.c		\
	print-weave.c		\
	print.h			\
	quickmatrix257.h	\
	ran.367.179.h		\
	\
	print-image-gimp.c	\
	print.c			\
	gimp_color_window.c	\
	gimp_main_window.c	\
	print_gimp.h

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
	-DVERSION=\"4.0a1\"			\
	-DRELEASE_DATE=\"11\ Aug\ 2000\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
