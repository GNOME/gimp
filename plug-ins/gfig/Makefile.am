## Process this file with automake to produce Makefile.in

SUBDIRS = gfig-examples

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = gfig

gfig_SOURCES = \
	gfig.c		\
	pix_data.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la		\
	$(GTK_LIBS)						\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui-@LT_RELEASE@.la	\
	$(top_builddir)/libgimp/libgimp-@LT_RELEASE@.la	

gfig_DEPENDENCIES = $(DEPS)

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
