## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = deinterlace

deinterlace_SOURCES = \
	deinterlace.c  $(top_srcdir)/plug-ins/megawidget/megawidget.h  

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	 $(top_srcdir)/plug-ins/megawidget/libmegawidget.a		\
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(X_LIBS)				\
	-lc

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	 $(top_srcdir)/plug-ins/megawidget/libmegawidget.a		\
	$(top_builddir)/libgimp/libgimp.la

deinterlace_DEPENDENCIES = $(DEPS)

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
