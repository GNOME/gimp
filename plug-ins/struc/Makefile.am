## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = struc

struc_SOURCES = \
	struc.c		\
	struc.h

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"

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
