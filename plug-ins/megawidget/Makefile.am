## Process this file with automake to produce Makefile.in

pluginlibdir = $(libdir)

pluginlib_LIBRARIES = libmegawidget.a

libmegawidget_a_SOURCES = \
	megawidget.c		\
	megawidget.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
