## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = rcm

rcm_SOURCES = \
	rcm.c		\
	rcm.h		\
	rcm_callback.c	\
	rcm_callback.h	\
	rcm_dialog.c	\
	rcm_dialog.h	\
	rcm_gdk.c	\
	rcm_gdk.h	\
	rcm_misc.c	\
	rcm_misc.h	\
	rcm_pixmaps.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

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
