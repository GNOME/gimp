## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = borderaverage

borderaverage_SOURCES = \
	borderaverage.c

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/plug-ins/libgck	\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/plug-ins/libgck/gck/libgck.la		\
	$(top_builddir)/plug-ins/megawidget/libmegawidget.a	\
	$(top_builddir)/libgimp/libgimpui.la              	\
	$(top_builddir)/libgimp/libgimp.la              	\
	$(GTK_LIBS)						\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
