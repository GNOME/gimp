## Process this file with automake to produce Makefile.in

pluginlibdir = $(libdir)

pluginlib_LIBRARIES = libgpc.a

libgpc_a_SOURCES = \
	gpc.c		\
	gpc.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimp.la	

libgpc_a_DEPENDENCIES = $(DEPS)

EXTRA_DIST=gpc.3 gpc.man

man_MANS=gpc.3

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
