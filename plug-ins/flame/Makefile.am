## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = README

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
	$(LIBUCB)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
