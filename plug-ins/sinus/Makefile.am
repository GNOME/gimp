## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = sinus

sinus_SOURCES = \
	sinus.c						\
	sinus_logo.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimp.la			\
	$(GTK_LIBS)						\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
