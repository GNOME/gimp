## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = fuse

fuse_SOURCES = \
	fuse.c

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
        -I$(top_srcdir)/plug-ins/libgck \
	-I$(includedir)

LDADD = \
	$(top_builddir)/plug-ins/megawidget/megawidget.o	\
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(top_builddir)/plug-ins/libgck/gck/libgck.la	\
	$(X_LIBS)				\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(top_builddir)/plug-ins/libgck/gck/libgck.la

fuse_DEPENDENCIES = $(DEPS)

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





