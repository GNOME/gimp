## Process this file with automake to produce Makefile.in

scriptdata =

pluginlibdir = $(gimpplugindir)/plug-ins

noinst_PROGRAMS = dgimp lgp

dgimp_SOURCES = \
	dgimp.c			\
	dgimp_invert.c		\
	dgimp_procs.h		\
	dgimp_utils.c

lgp_SOURCES = \
	lgp.c			\
	lgp_invert.c		\
	lgp_procs.h

EXTRA_DIST = README

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

dgimp_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(X_LIBS)				\
	-lc

lgp_LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(X_LIBS)				\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimp.la

dgimp_DEPENDENCIES = $(DEPS)
lgp_DEPENDENCIES = $(DEPS)

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
