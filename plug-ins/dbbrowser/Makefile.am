## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = README

pluginlib_PROGRAMS = dbbrowser

dbbrowser_SOURCES = \
	dbbrowser.c		\
	dbbrowser.h		\
	dbbrowser_utils.c	\
	dbbrowser_utils.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimp.la	

dbbrowser_DEPENDENCIES = $(DEPS)

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
