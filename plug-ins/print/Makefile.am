## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

print_SOURCES = \
        print.c			\
        print-image-gimp.c	\
        gimp_color_window.c	\
        gimp_main_window.c	\
        print_gimp.h

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	$(GTK_CFLAGS)		\
	$(PRINT_CFLAGS)		\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(PRINT_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
