## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = flame

flame_SOURCES = \
	cmap.c		\
	cmap.h		\
	flame.c		\
	flame.h		\
	libifs.c	\
	libifs.h	\
	megawidget.c	\
	megawidget.h	\
	rect.c		\
	rect.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(LIBUCB_LIB)				\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	

flame_DEPENDENCIES = $(DEPS)

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
