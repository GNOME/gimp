## Process this file with automake to produce Makefile.in

SUBDIRS = 			\
	@TIFF@			\
	gbr			\
	pat			\
	dbbrowser		\
	script-fu		\
	sgi

scriptdata =

pluginlibdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = build.in

CPPFLAGS = \
        -DPRECISION_CONFIG=$(precision)

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LIBGIMPUI = $(top_builddir)/libgimp/libgimpui.la

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
