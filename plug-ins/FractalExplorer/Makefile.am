## Process this file with automake to produce Makefile.in

SUBDIRS = fractalexplorer-examples

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = FractalExplorer

FractalExplorer_SOURCES = \
	Callbacks.h		\
	Callbacks.c		\
	Dialogs.h		\
	Dialogs.c		\
	Events.h		\
	Events.c		\
	FractalExplorer.c	\
	FractalExplorer.h	\
	Globals.c		\
	Languages.c		\
	Languages.h		\
	logo.h			\
	pix_data.h

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
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
