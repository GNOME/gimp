## Process this file with automake to produce Makefile.in

scriptdatadir = $(gimpdatadir)/scripts

scriptdata_DATA = web-browser.scm

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = webbrowser

webbrowser_SOURCES = \
	webbrowser.c   

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
	$(LIBXMU)				\
	$(INTLLIBS)

EXTRA_DIST = $(scriptdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
