## Process this file with automake to produce Makefile.in

scriptdatadir = $(gimpdatadir)/scripts

scriptdata_DATA = web-browser.scm

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = webbrowser

webbrowser_SOURCES = \
	webbrowser.c   

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(X_LIBS)				\
	@LIBXMU_LIB@

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la

webbrowser_DEPENDENCIES = $(DEPS)

EXTRA_DIST = $(scriptdata_DATA)

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
