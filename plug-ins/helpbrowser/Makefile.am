## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = helpbrowser

helpbrowser_SOURCES = \
	back.xpm		\
	forward.xpm		\
	helpbrowser.c		\
	queue.c			\
	queue.h

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTKXMHTML_CFLAGS)		\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimp.la	\
	$(top_builddir)/libgimp/libgimpui.la	\
	$(GTKXMHTML_LIBS)			\
	$(INTLLIBS)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
