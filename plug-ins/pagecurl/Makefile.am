## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = pagecurl

pagecurl_SOURCES = \
	pagecurl.c	\
	curl0.xpm	\
	curl1.xpm	\
	curl2.xpm	\
	curl3.xpm	\
	curl4.xpm	\
	curl5.xpm	\
	curl6.xpm	\
	curl7.xpm

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
