## Process this file with automake to produce Makefile.in

SUBDIRS = gflares

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gflare

gflare_SOURCES = \
	asupsample.h		\
	asupsample.c		\
	gtkmultioptionmenu.h	\
	gtkmultioptionmenu.c	\
	gflare.c

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
