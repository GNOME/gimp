## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = autostretch_hsv

autostretch_hsv_SOURCES = \
	autostretch_hsv.c   

INCLUDES = \
	-I$(top_srcdir)			\
	$(GLIB_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GLIB_LIBS)				\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimp.la

autostretch_hsv_DEPENDENCIES = $(DEPS)

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
