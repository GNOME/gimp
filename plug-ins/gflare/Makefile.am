## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = README

pluginlib_PROGRAMS = gflare

gflare_SOURCES = \
	asupsample.c		\
	asupsample.h		\
	gflare.c		\
	gtkmultioptionmenu.c	\
	gtkmultioptionmenu.h

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la			\
	$(top_builddir)/libgimp/libgimp.la			\
	$(X_LIBS)						\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la			\
	$(top_builddir)/libgimp/libgimp.la

gflare_DEPENDENCIES = $(DEPS)

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
