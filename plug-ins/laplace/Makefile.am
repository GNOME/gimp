## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = laplace

laplace_SOURCES = \
	laplace.c   

INCLUDES = \
	-I$(top_srcdir)			\
	$(GLIB_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimp.la	

laplace_DEPENDENCIES = $(DEPS)

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
